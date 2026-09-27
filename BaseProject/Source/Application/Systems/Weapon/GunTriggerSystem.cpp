#include "GunTriggerSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Weapon/WeaponTriggerComponent.h"
#include "Application/Components/Weapon/GunStateComponent.h"
#include "Application/Components/Weapon/GunFireComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Combat/AimResultComponent.h"
#include "Application/Components/Render/ModelComponent.h"

//==========================================================================================
// GunTriggerSystem
//
// 引き金(WeaponTriggerComponent::isPulled)を受けて、このフレームに撃つかを決め、
// 撃つなら銃口の位置と射出方向を GunFireComponent へ書く。弾は出さない。
//
// ・引き金を引いているかどうかしか外からは来ない。
//   実際に撃てるかどうか(連射間隔・バースト・オーバーヒート)は武器側のここが決める。
//   撃ち方は Auto(押しっぱなしで連射)と Burst(まとめて数発)の2種類。
// ・書くのは自分の GunState / GunFire だけで、狙点(AimTargetPos)とモデルは読むだけなので、
//   チャンクを分けてワーカーで回す。
// ・弾の生成は GunProjectileSpawnSystem、銃口の光は MuzzleFlashSystem(どちらもメインスレッド)。
//   以前はまとめて GunShootSystem が行っていた。
//==========================================================================================
void GunTriggerSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveJobTask<GunStateComponent, GunFireComponent, const WeaponTriggerComponent,
		const WorldMatrixComponent, const ModelComponent>(
		Engine::ECS::ESystemType::Update,
		"GunTriggerSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*,
			GunStateComponent* a_gunArray,
			GunFireComponent* a_fireArray,
			const WeaponTriggerComponent* a_triggerArray,
			const WorldMatrixComponent* a_worldMatArray,
			const ModelComponent* a_modelArray
		)
		{
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				GunStateComponent& _gun = a_gunArray[_i];
				GunFireComponent& _fireComp = a_fireArray[_i];
				const WeaponTriggerComponent& _trigger = a_triggerArray[_i];

				// 撃たなかったフレームは下ろしておく(読み手はこの印だけを見る)
				_fireComp.isFired = false;

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

				// プレハブ未設定なら撃たない(間隔と熱だけは進めてある)
				if (_gun.bulletPrefabGUID == Engine::DefaultGUID) continue;

				// ---- 銃の位置と、銃自身のローカル +Z 軸 ----
				const Math::Matrix& _m = a_worldMatArray[_i].worldMat;
				const Math::Vector3 _pos = { _m._41, _m._42, _m._43 };	// 平行移動
				Math::Vector3 _gunFwd = { _m._31, _m._32, _m._33 };		// ローカル +Z 軸

				const float _gunFwdLenSq = _gunFwd.LengthSquared();
				if (_gunFwdLenSq > 1e-8f) _gunFwd /= std::sqrt(_gunFwdLenSq);

				//======================================================================
				// 基準の向きを決める
				//----------------------------------------------------------------------
				// 銃はアニメーションノードに追従しているため、ローカル +Z 軸が実際の
				// 銃口方向とは限らない(ボーンの軸がそのまま出る)。
				// そのため狙点がある時は、銃の軸ではなく「狙いの向き(カメラ前方)」を
				// 銃口オフセットと後方判定の基準にする。
				//
				// AimResultComponent は AimTargetSystem が計算し、
				// AttachmentDispatchSystem が親から配信してくる。
				// 付いていない銃は自分の +Z 軸へ撃つ。
				//======================================================================
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				const AimResultComponent* _pAim = a_ctx.pWorld->RefData<AimResultComponent>(_self);

				// まだ一度も計算されていない(=原点が入っている)なら使わない
				if (_pAim && !_pAim->isValid) _pAim = nullptr;

				Math::Vector3 _baseDir = _gunFwd;
				if (_pAim)
				{
					const Math::Vector3 _aimDir = Math::Vector3(_pAim->dir);
					const float _aimDirLenSq = _aimDir.LengthSquared();
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
				Math::Vector3 _muzzleLocalPos = { 0.0f, 0.0f, 0.0f };
				Math::Vector3 _spawnPos = _pos;
				if (_gun.nullPtrNodeHash != 0)
				{
					const auto* _pModel = a_ctx.pServices->pResourceManager->Get(a_modelArray[_i].handle);
					if (_pModel)
					{
						const auto& _nodeVec = _pModel->GetOriginalNodeVec();
						if (_gun.nodeIndex < _nodeVec.size())
						{
							const Math::Matrix& _nodeMat = _nodeVec[_gun.nodeIndex].worldTransform;
							const Math::Vector3 _nodeLocalPos = { _nodeMat._41, _nodeMat._42, _nodeMat._43 };
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

				_fireComp.isFired        = true;
				_fireComp.spawnPos       = _spawnPos;
				_fireComp.shootDir       = _shootDir;
				_fireComp.muzzleLocalPos = _muzzleLocalPos;
			}
		}
	)
	// 絞り込みに使わない読み : 自分の狙点(AttachmentDispatchSystem が配る)
	.Reads<AimResultComponent>();
}
