#include "EffectUpdateSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Effect/EffectAssetComponent.h"
#include "../../../../../Components/Transform/WorldMatrixComponent.h"

//==========================================================================================
// EffectUpdateSystem
//
// EffectAssetComponent の isPlay を見て、エフェクトの時間を進める。
//
// ・isPlay の立ち上がりで頭から再生、立ち下がりで停止。
//   制御側は毎フレーム isPlay を書くだけでよく、
//   「始まった瞬間」を自分で数えなくてよい。
// ・時間を進めた結果として、このフレームの発生数(pendingEmit)がパーツごとに決まる。
//   実際の発生要求と描画は EffectDrawSystem(Draw)が行う。
// ・サウンドパーツを鳴らすのもここ(時間を持っているのがこのシステムのため)。
//   3D指定のものが正しい位置で鳴るよう、鳴らす前に発生源の座標を入れておく。
//
// 実フレーム時間(a_ctx.dt)が要るので Update フェーズで回す
// (Draw フェーズは dt = 0 のため、ここでやらないと時間が進まない)。
//==========================================================================================
void EffectUpdateSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<EffectAssetComponent>(
		Engine::ECS::ESystemType::Update,
		"EffectUpdateSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			EffectAssetComponent* a_effectArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			auto* _pAudioManager = a_ctx.pServices->pAudioManager;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				EffectAssetComponent& _comp = a_effectArray[_i];

				auto* _pEffect = _pResourceManager->Ref(_comp.effectHandle);
				if (!_pEffect) continue;

				//----------------------------------------------------------
				// 3Dで鳴らすサウンドパーツの位置
				//
				// 音の発生源はエフェクトが付いている相手の居場所なので、
				// 鳴らす前に入れておく。鳴っている最中の音にも即時反映される。
				//
				// WorldMatrixComponent はクエリに入れず RefData で引く。
				// クエリに足すと、行列を持たないエフェクトが丸ごと対象から外れて
				// 時間すら進まなくなるため。
				//
				// RefData は持っていないコンポーネントでも非nullを返すので、
				// 必ず HasComponent で確かめてから引くこと
				//----------------------------------------------------------
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (_pAudioManager && a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_self))
				{
					if (const auto* _pWorldMat = a_ctx.pWorld->RefData<WorldMatrixComponent>(_self))
					{
						const Math::Matrix _world(_pWorldMat->worldMat);
						_comp.instance.SetSoundPos(*_pAudioManager, _world.Translation());
					}
				}

				// ---- 再生 / 停止の切り替え ----
				// アセット側の実体が「再生中か」を覚えているので、
				// 要求(isPlay)との差を見れば立ち上がり・立ち下がりが分かる
				if (_comp.isPlay && !_comp.instance.isPlaying)
				{
					_pEffect->Play(_comp.instance);
					ENGINE_LOG("[DBG-Update] Play (dt=%.4f)", a_ctx.dt);
				}
				else if (!_comp.isPlay && _comp.instance.isPlaying)
				{
					// 音も一緒に止める(止めるのはループを掛けたものだけ)
					_pEffect->Stop(_comp.instance, _pAudioManager);
				}

				// ---- 時間を進めて、このフレームの発生数を決める ----
				// 時間が来たサウンドパーツを鳴らすのもこの中
				_pEffect->Update(_comp.instance, a_ctx.dt, _pAudioManager);

				// ---- 出し切ったら自分ごと消す ----
				// 解放予約だけしておく。実際に消えるのは次の BeginFrame で、
				// その前に Release フェーズが走るので借りているものは返ってから消える
				// (音が鳴り終わるまで待つかはサウンドパーツの isWaitFinish 次第)
				if (_comp.destroyOnFinish && _pEffect->IsFinished(_comp.instance, _pAudioManager))
				{
					ENGINE_LOG("[DBG-Update] Finish -> release (elapsed=%.3f)", _comp.instance.elapsed);
					a_ctx.pWorld->AddReleaseEntity(a_pChunk->entityData[_i]);
				}
				else if (_comp.instance.pendingEmit[0] > 0 || _comp.instance.pendingEmit[1] > 0)
				{
					ENGINE_LOG("[DBG-Update] elapsed=%.3f pending=%d/%d",
						_comp.instance.elapsed,
						_comp.instance.pendingEmit[0], _comp.instance.pendingEmit[1]);
				}
			}
		}
	);
}
