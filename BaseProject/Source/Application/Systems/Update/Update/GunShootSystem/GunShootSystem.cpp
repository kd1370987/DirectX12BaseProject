#include "GunShootSystem.h"

#include "Application/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../../../../Components/Character/Weapon/WeaponTriggerComponent.h"
#include "../../../../Components/Character/Weapon/Gun/GunStateComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Character/AimTargetPosComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Character/Weapon/Projectile/HomingComponent.h"
#include "../../../../Components/Character/Weapon/Projectile/ProjectileComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"
#include "../../../../Components/Collision/Collider.h"

#include "../../../Shared/ProjectileSpawn/ProjectileSpawn.h"
#include "../../../../Components/Effect/EffectAssetComponent.h"

//==========================================================================================
// GunShootSystem
//
// GunStateComponent を持つエンティティが、引き金(WeaponTriggerComponent::isPulled)に
// 応じて、設定されたプレハブを「弾」として生成する。
//
// 引き金を引いているかどうかしか外からは来ない。
// 実際に撃てるかどうか(連射間隔・バースト・オーバーヒート)も、
// 何をどう撃つか(弾・弾速・銃口)も、すべて武器側のこのシステムが決める。
// 撃ち方は Auto(押しっぱなしで連射)と Burst(まとめて数発)の2種類。
// 生成はシステム反復中に即時に行えない(アーキタイプが壊れる)ため、
// World の遅延生成コマンド(AddEntityWithData)に積み、BeginFrame で安全に生成する。
//
// 1発撃つごとに、銃口へマズルフラッシュ(EffectAsset)も出す。
// 弾と同じ位置・同じ向きへ単発で出すだけなので、銃に付けて追従させてはいない。
//
// プレハブが HomingComponent を持っていた場合は、ここで「追う相手」を埋める。
// 発射した後から相手を探すのではなく、撃った瞬間に撃った側が捉えている相手を
// 弾へ渡す形にしている(敵の索敵結果 = TargetEntityComponent がそのまま弾の的になる)。
// 実際に曲げるのは HomingSystem。
//==========================================================================================
namespace
{
	//======================================================================================
	// 撃った側が「今どのエンティティを狙っているか」を解決する
	//
	//   1) 自分 → 親 と辿って TargetEntityComponent(索敵結果)を探す。
	//      銃はアタッチメントの子エンティティになっていることがあるので、
	//      索敵している本体(敵キャラ)は親側にいる。
	//   2) 見つからなければ狙点(レティクル)が当たっている相手を使う。
	//      プレイヤーが誘導弾を撃った時はこちらが拾われる。
	//
	// 誰も狙っていなければ無効値を返す(＝誘導せずに直進する弾になる)。
	//======================================================================================
	Engine::ECS::Entity ResolveHomingTarget(
		Engine::ECS::World&          a_world,
		Engine::ECS::Entity          a_shooter,
		const AimTargetPosComponent* a_pAim)
	{
		// 親を辿る深さの上限。親子が循環していても止まるように付けておく
		constexpr int _kMaxDepth = 8;

		Engine::ECS::Entity _entity = a_shooter;
		for (int _d = 0; _d < _kMaxDepth; ++_d)
		{
			if (_entity == Engine::ECS::Limits::INVALID_ENTITY) break;

			if (a_world.HasComponent<TargetEntityComponent>(_entity))
			{
				const auto* _pTarget = a_world.RefData<TargetEntityComponent>(_entity);

				// 見失っている間の的は信用しない(古い位置を追ってしまうため)
				if (_pTarget && _pTarget->isFind &&
					_pTarget->targetEntity != Engine::ECS::Limits::INVALID_ENTITY)
				{
					return _pTarget->targetEntity;
				}
			}

			// 親へ
			if (!a_world.HasComponent<HierarchyComponent>(_entity)) break;
			const auto* _pHierarchy = a_world.RefData<HierarchyComponent>(_entity);
			if (!_pHierarchy) break;
			_entity = _pHierarchy->parentID;
		}

		// 狙点が何かに当たっているなら、それを追わせる
		if (a_pAim && a_pAim->isHit) return a_pAim->hitEntity;

		return Engine::ECS::Limits::INVALID_ENTITY;
	}

	// 発射元の解決(自分 → 親と辿ってコライダー持ちを探す)は
	// ミサイルと共通なので App::Systems::ProjectileSpawn へ寄せてある。
}

void GunShootSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<GunStateComponent, const WeaponTriggerComponent, const WorldMatrixComponent,
		const ModelComponent>(
		Engine::ECS::ESystemType::Update,
		"GunShootSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			GunStateComponent* a_gunArray,
			const WeaponTriggerComponent* a_triggerArray,
			const WorldMatrixComponent* a_worldMatArray,
			const ModelComponent* a_modelArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				GunStateComponent& _gun = a_gunArray[_i];
				const WeaponTriggerComponent& _trigger = a_triggerArray[_i];
				const WorldMatrixComponent& _worldMat = a_worldMatArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];

				// 前回発射からの経過時間を進める。
				// 撃っていない間もステートが変わっても止めずに足し続けるので、
				// 「久しぶりに引き金を引いたら即撃てる」形になる
				_gun.timeSinceShoot += a_ctx.dt;

				//======================================================================
				// 冷却
				//----------------------------------------------------------------------
				// 撃っていてもいなくても毎フレーム冷ます。
				// オーバーヒート中は overheatCoolScale を掛けて冷えを鈍らせられる
				// (無理をした分だけ復帰を待たされる、というペナルティ)。
				// 熱が復帰しきい値まで下がったらまた撃てるようにする。
				//======================================================================
				if (_gun.useOverheat)
				{
					const float _coolScale = _gun.isOverheat ? _gun.overheatCoolScale : 1.0f;
					_gun.heat = (std::max)(_gun.heat - _gun.heatCoolRate * _coolScale * a_ctx.dt, 0.0f);

					if (_gun.isOverheat && _gun.heat <= _gun.heatLimit * _gun.restartHeatRatio)
					{
						_gun.isOverheat = false;
					}
				}
				else
				{
					// 途中で設定を切られたときに熱が残り続けないようにしておく
					_gun.heat = 0.0f;
					_gun.isOverheat = false;
				}

				// 次弾までの間隔(秒) = 1 / 発射レート
				const float _shotInterval = (_gun.fireRate > 0.0f) ? (1.0f / _gun.fireRate) : 0.0f;

				// 引き金が引かれていても、熱が上限に達している間は反応しない
				const bool _canPull = _trigger.isPulled && !_gun.isOverheat;

				//======================================================================
				// 発射判定
				//   Auto  : 押している間、発射レートの間隔で撃ち続ける
				//   Burst : 一度始まったらトリガーを離しても burstCount 発を撃ち切り、
				//           撃ち切ったら burstInterval あけて次のバーストへ
				//======================================================================
				bool _fire = false;
				if (_gun.fireMode == EFireMode::Burst)
				{
					if (_gun.burstRemain > 0)
					{
						// バースト継続中。残弾はレート間隔で撃つ
						_fire = (_gun.timeSinceShoot >= _shotInterval);
					}
					else if (_canPull)
					{
						// 次のバーストを始められるか(前回発射からの間隔で見る)
						_fire = (_gun.timeSinceShoot >= _gun.burstInterval);
						if (_fire) _gun.burstRemain = (_gun.burstCount > 0) ? _gun.burstCount : 1;
					}
				}
				else
				{
					_fire = _canPull && (_gun.timeSinceShoot >= _shotInterval);
				}

				if (!_fire) continue;

				// 撃つと決めた時点で数える。この後プレハブが無くて撃てなくても、
				// 間隔の計算とバーストの進行は進める
				_gun.timeSinceShoot = 0.0f;
				if (_gun.burstRemain > 0) --_gun.burstRemain;

				// 熱を溜める。上限に届いたらオーバーヒート。
				// バーストの途中でも撃ち切らせずに止める(熱が尽きたら撃てない、を優先する)
				if (_gun.useOverheat)
				{
					_gun.heat += _gun.heatPerShot;
					if (_gun.heat >= _gun.heatLimit)
					{
						_gun.heat = _gun.heatLimit;
						_gun.isOverheat = true;
						_gun.burstRemain = 0;
					}
				}

				// プレハブ未設定ならスキップ
				if (_gun.bulletPrefabGUID == Engine::DefaultGUID) continue;

				// プレハブのハンドルを解決(未ロードならロード)
				auto& _rm = *a_ctx.pServices->pResourceManager;
				if (!_rm.IsValid(_gun.bulletPrefabHandle))
				{
					// 取った参照は銃が消えるときに返す
					// (GunStateComponent の解放フック)
					_rm.AcquireImmediate(_gun.bulletPrefabHandle, _gun.bulletPrefabGUID);
				}
				auto* _pPrefab = _rm.Ref(_gun.bulletPrefabHandle);
				if (!_pPrefab) continue;

				// ---- 銃の位置と、銃自身のローカル +Z 軸 ----
				const Math::Matrix& _m = _worldMat.worldMat;
				Math::Vector3 _pos = { _m._41, _m._42, _m._43 };	// 平行移動
				Math::Vector3 _gunFwd = { _m._31, _m._32, _m._33 };	// ローカル +Z 軸

				float _gunFwdLenSq = _gunFwd.LengthSquared();
				if (_gunFwdLenSq > 1e-8f) _gunFwd /= std::sqrt(_gunFwdLenSq);

				//======================================================================
				// 基準の向きを決める
				//----------------------------------------------------------------------
				// 銃はアニメーションノードに追従しているため、ローカル +Z 軸が実際の
				// 銃口方向とは限らない(ボーンの軸がそのまま出る)。
				// そのため狙点がある時は、銃の軸ではなく「狙いの向き(カメラ前方)」を
				// 銃口オフセットと後方判定の基準にする。
				//
				// AimTargetPosComponent は AimTargetSystem が計算し、
				// AttachmentDispatchSystem が親から配信してくる。
				// 付いていない銃は今まで通り自分の +Z 軸へ撃つ。
				//======================================================================
				const AimTargetPosComponent* _pAim = nullptr;
				Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_self))
				{
					_pAim = a_ctx.pWorld->RefData<AimTargetPosComponent>(_self);

					// まだ一度も計算されていない(=原点が入っている)なら使わない
					if (_pAim && !_pAim->isValid) _pAim = nullptr;
				}

				Math::Vector3 _baseDir = _gunFwd;
				if (_pAim)
				{
					Math::Vector3 _aimDir = Math::Vector3(_pAim->dir);
					float _aimDirLenSq = _aimDir.LengthSquared();
					if (_aimDirLenSq > 1e-8f) _baseDir = _aimDir / std::sqrt(_aimDirLenSq);
				}

				//======================================================================
				// 発射位置 : 銃口ヌルノードが設定されていればそこから撃つ
				//----------------------------------------------------------------------
				// node.worldTransform は「モデルルート基準」の行列なので、その平行移動
				// 成分はモデル空間の値。銃の向き・スケールを反映させるため、
				// エンティティのワールド行列で変換してワールド座標にする。
				// (ノードインデックスは GunStateStartSystem がハッシュから解決する)
				//======================================================================
				// 銃ローカルの銃口位置。マズルフラッシュの発生位置に使う
				// (エフェクトは銃自身に付いているので、渡すのは銃基準のオフセット)
				Math::Vector3 _muzzleLocalPos = { 0.0f, 0.0f, 0.0f };

				Math::Vector3 _spawnPos = _pos;
				if (_gun.nullPtrNodeHash != 0)
				{
					auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
					if (_pModel)
					{
						const auto& _nodeVec = _pModel->GetOriginalNodeVec();
						if (_gun.nodeIndex < _nodeVec.size())
						{
							const Math::Matrix& _nodeMat = _nodeVec[_gun.nodeIndex].worldTransform;
							Math::Vector3 _nodeLocalPos = { _nodeMat._41, _nodeMat._42, _nodeMat._43 };
							_muzzleLocalPos = _nodeLocalPos;
							_spawnPos = Math::Vector3::Transform(_nodeLocalPos, Math::Matrix(_m));
						}
					}
				}

				//======================================================================
				// 射出方向 : 銃口から狙点へ向ける
				//======================================================================
				Math::Vector3 _shootDir = _baseDir;
				if (_pAim)
				{
					Math::Vector3 _toTarget = Math::Vector3(_pAim->pos) - _spawnPos;

					// 狙点が銃口とほぼ同じ位置だと向きが定まらないので、その時は基準のまま
					if (_toTarget.LengthSquared() > 1e-6f)
					{
						_toTarget.Normalize();

						// 狙点が真後ろにある場合は採用しない。
						// (自機の手前の物を拾ってしまった時に、弾がカメラへ向かって
						//  飛んでいくのを防ぐための保険。基準は必ず狙いの向き)
						if (_toTarget.Dot(_baseDir) > 0.0f)
						{
							_shootDir = _toTarget;
						}
					}
				}

				Math::Vector3 _velValue = _shootDir * _gun.speed;

				// 生成はミサイルと共通のヘルパーへ(遅延生成コマンドに積まれる)
				App::Systems::ProjectileSpawn::Spawn(
					*a_ctx.pWorld,
					_pPrefab,
					_spawnPos,
					_velValue,
					App::Systems::ProjectileSpawn::ResolveShooterEntity(*a_ctx.pWorld, _self),
					ResolveHomingTarget(*a_ctx.pWorld, _self, _pAim));

				//======================================================================
				// マズルフラッシュ
				//----------------------------------------------------------------------
				// 銃口の位置・射出方向へ、銃自身が持っている再生枠を頭から再生し直す。
				//
				// エフェクト用のエンティティは出さない。出すとそのエンティティは
				// エフェクトが終わるまでワールドに残るので、移動しながら撃つと
				// 撃った位置に取り残されて尾を引いて見える。
				// (枠を持たせるのは GunStateStartSystem)
				//
				// 置き方は毎発入れ直す。銃口ノードは変わらなくても、
				// 狙いの向きは撃つたびに変わるため。
				//
				// 撃つと決まった後に置いているので、
				// 弾が出なかったフレーム(プレハブ未設定など)では光らない
				//======================================================================
				if (_gun.muzzleEffectGUID != Engine::DefaultGUID &&
					a_ctx.pWorld->HasComponent<EffectAssetComponent>(_self))
				{
					auto* _pMuzzleComp = a_ctx.pWorld->RefData<EffectAssetComponent>(_self);
					auto* _pMuzzleEffect = _pMuzzleComp
						? Engine::Resource::ResourceManager::Instance().Ref(_pMuzzleComp->effectHandle)
						: nullptr;

					if (_pMuzzleEffect)
					{
						// 位置も向きも銃のローカルへ直して渡す。
						// EffectDrawSystem がこの銃のワールド行列を掛けるので、
						// ワールドのまま渡すと二重に掛かる
						const Math::Matrix _gunInvMat = Math::Matrix(_m).Invert();

						_pMuzzleComp->effectScale = _gun.muzzleEffectScale;
						_pMuzzleComp->isOverrideTransform = true;
						_pMuzzleComp->overridePosOffset = _muzzleLocalPos;
						_pMuzzleComp->overrideEmitDir =
							Math::Vector3::TransformNormal(_shootDir, _gunInvMat);

						// 頭から出し直す(Play は中で Reset を呼ぶ)。
						//
						// isPlay は一度立てたら下ろさない。EffectUpdateSystem は
						// isPlay の立ち下がりで Stop するので、下ろすと消えてしまう。
						// また立ち上がりの Play は「まだ再生していない」ときしか走らず、
						// 連射の上書きには使えないので、ここで直接 Play を呼ぶ。
						// 出し切った後はどのパーツも出す時間帯から外れるだけなので、
						// 再生中のまま置いておいても何も出ない
						_pMuzzleComp->isPlay = true;
						_pMuzzleEffect->Play(_pMuzzleComp->instance);
					}
				}
			}
		}
	);
}
