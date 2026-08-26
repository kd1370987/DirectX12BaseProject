#include "FlyingSoundSystem.h"

#include "Application/ECS/World/World.h"
#include "Engine/Audio/AudioManager.h"

#include "Application/Components/Character/FlyingSound.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/InstanceResource/FlyingSoundResource.h"

//==============================================================================
// FlyingSoundSystem
//
// 飛んでいる間ずっと鳴る音(ミサイルの飛翔音など)を、その位置で3D再生し続ける。
//
// ・毎フレームの流れ
//     1) 預かっているボイスを全部「見かけていない」に戻す
//     2) 生きているエンティティを回り、初回ならボイスを発行して Play3D、
//        2回目以降は SetPos で位置だけ更新(内部で Apply3D される)。見かけた印を付ける
//     3) 印が付かなかったボイス = 持ち主が消えた ので停止・返却する
//
// ・3) が必要な理由
//     自滅するエンティティ(寿命切れ・着弾)が Release フェーズを通らなかった頃の作りで、
//     コンポーネント側にハンドルを持たせて返却させることができなかったため、
//     「今フレーム見かけたか」で生死を判定して回収している。
//     (現在は自滅も AddReleaseEntity 経由なので、Release で返す形にも寄せられる)
//     エンティティを直接引いて生存確認しないのは、添え字が再利用されると
//     別のエンティティを本人と誤認してしまうため(ここでは世代込みの Entity をキーにしている)。
//
// ・コンポーネントを回すだけでは 3) を回せない(チャンクごとにコールバックが呼ばれるため)。
//   1フレームに1回だけ走らせたいのでカスタムタスクで登録する。
// ・位置はワールド行列から取る。リスナー側(AudioListenerSystem)と同じ PostUpdate 帯。
//==============================================================================
void FlyingSoundSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<FlyingSoundComponent, WorldMatrixComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;

			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			if (!a_ctx.pWorld->HasResource<FlyingSoundResource>()) return;
			auto& _res = a_ctx.pWorld->GetResource<FlyingSoundResource>();

			// ---- 1) 生存フラグを落とす ----
			_res.ResetAliveFlags();

			// ---- 2) 生きているエンティティのぶんを鳴らす / 位置を更新する ----
			a_ctx.pWorld->ForEach<ActiveTag, FlyingSoundComponent, WorldMatrixComponent>(
				[&_res, _pAudioManager](
					Engine::ECS::ArchetypeChunk* a_pChunk,
					uint32_t                     a_count,
					ActiveTag*                   a_tags,
					FlyingSoundComponent*        a_flyingSoundArray,
					WorldMatrixComponent*        a_worldMatArray)
				{
					for (uint32_t _i = 0; _i < a_count; ++_i)
					{
						const FlyingSoundComponent& _flyingSound = a_flyingSoundArray[_i];
						const WorldMatrixComponent& _worldComp   = a_worldMatArray[_i];

						if (_flyingSound.soundGUID == Engine::DefaultGUID) continue;

						Engine::ECS::Entity _self = a_pChunk->entityData[_i];
						Math::Vector3 _pos = Math::Matrix(_worldComp.worldMat).Translation();

						FlyingSoundVoice* _pVoice = _res.Find(_self);

						// ---- 初回 : ボイスを発行して鳴らし始める ----
						if (!_pVoice)
						{
							// 3D指定で発行しないと Play3D / SetPos が使えない
							auto _handle = _pAudioManager->RequestSoundInstance(_flyingSound.soundGUID, true);

							// 発行に失敗(サウンドが見つからない等)しても預けておく。
							// 毎フレーム作り直しに行かないため。無効ハンドルの返却は空振りするだけ
							_res.Add(_self, _handle);

							auto* _pInstance = _pAudioManager->RefInstance(_handle);
							if (!_pInstance) continue;

							_pInstance->SetCurveDistanceScaler(_flyingSound.distanceScaler);
							_pInstance->Play3D(_pos, _flyingSound.isLoop);

							// Play3D は覚えている音量で鳴らし直すので、ここで指定しておく
							_pInstance->SetVolume(_flyingSound.vol);
							continue;
						}

						// ---- 2回目以降 : 位置だけ更新(内部で Apply3D される) ----
						_pVoice->isAlive = true;

						auto* _pInstance = _pAudioManager->RefInstance(_pVoice->handle);
						if (!_pInstance) continue;

						_pInstance->SetPos(_pos);
					}
				}
			);

			// ---- 3) 持ち主が消えたボイスを止めて返却する ----
			for (auto _it = _res.voiceMap.begin(); _it != _res.voiceMap.end(); )
			{
				if (_it->second.isAlive)
				{
					++_it;
					continue;
				}

				// ReleaseSoundInstance が停止も行う
				_pAudioManager->ReleaseSoundInstance(_it->second.handle);
				_it = _res.voiceMap.erase(_it);
			}
		}
	);
}
