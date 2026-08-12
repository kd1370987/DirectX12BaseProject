#include "GunShootSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../../../../Components/Intent/ActionIntentComponent.h"
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

//==========================================================================================
// GunShootSystem
//
// GunStateComponent を持つエンティティが、発射入力(ActionIntentComponent::isGunShoot)に
// 応じて、設定されたプレハブを「弾」として生成する。
// 撃ち方は Auto(押しっぱなしで連射)と Burst(まとめて数発)の2種類。
// 生成はシステム反復中に即時に行えない(アーキタイプが壊れる)ため、
// World の遅延生成コマンド(AddEntityWithData)に積み、BeginFrame で安全に生成する。
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

	//======================================================================================
	// 「発射元」として弾に持たせるエンティティを解決する
	//
	// 銃は本体にぶら下がる子エンティティのことがあり、コライダーを持つのは本体側
	// (銃自体はコリジョンワールドに登録されていない)。弾が当たらないようにしたいのは
	// 本体なので、自分 → 親 と辿って最初に ColliderComponent を持つものを発射元とする。
	// 銃が本体そのもの(敵など)なら自分がそのまま返る。
	// 誰もコライダーを持たなければ、辿れた最上位を返しておく。
	//======================================================================================
	Engine::ECS::Entity ResolveShooterEntity(
		Engine::ECS::World& a_world,
		Engine::ECS::Entity a_gunEntity)
	{
		// 親を辿る深さの上限。親子が循環していても止まるように付けておく
		constexpr int _kMaxDepth = 8;

		Engine::ECS::Entity _entity = a_gunEntity;
		Engine::ECS::Entity _last   = a_gunEntity;

		for (int _d = 0; _d < _kMaxDepth; ++_d)
		{
			if (_entity == Engine::ECS::Limits::INVALID_ENTITY) break;
			_last = _entity;

			// コライダーを持つ = コリジョンワールドに居る本体
			if (a_world.HasComponent<ColliderComponent>(_entity)) return _entity;

			// 親へ
			if (!a_world.HasComponent<HierarchyComponent>(_entity)) break;
			const auto* _pHierarchy = a_world.RefData<HierarchyComponent>(_entity);
			if (!_pHierarchy) break;
			_entity = _pHierarchy->parentID;
		}

		return _last;
	}
}

void GunShootSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<GunStateComponent, const ActionIntentComponent, const WorldMatrixComponent,
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
			const ActionIntentComponent* a_intentArray,
			const WorldMatrixComponent* a_worldMatArray,
			const ModelComponent* a_modelArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				GunStateComponent& _gun = a_gunArray[_i];
				const ActionIntentComponent& _intent = a_intentArray[_i];
				const WorldMatrixComponent& _worldMat = a_worldMatArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];

				// 前回発射からの経過時間を進める。
				// 撃っていない間もステートが変わっても止めずに足し続けるので、
				// 「久しぶりに引き金を引いたら即撃てる」形になる
				_gun.timeSinceShoot += a_ctx.dt;

				// 次弾までの間隔(秒) = 1 / 発射レート
				const float _shotInterval = (_gun.fireRate > 0.0f) ? (1.0f / _gun.fireRate) : 0.0f;

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
					else if (_intent.isGunShoot)
					{
						// 次のバーストを始められるか(前回発射からの間隔で見る)
						_fire = (_gun.timeSinceShoot >= _gun.burstInterval);
						if (_fire) _gun.burstRemain = (_gun.burstCount > 0) ? _gun.burstCount : 1;
					}
				}
				else
				{
					_fire = _intent.isGunShoot && (_gun.timeSinceShoot >= _shotInterval);
				}

				if (!_fire) continue;

				// 撃つと決めた時点で数える。この後プレハブが無くて撃てなくても、
				// 間隔の計算とバーストの進行は進める
				_gun.timeSinceShoot = 0.0f;
				if (_gun.burstRemain > 0) --_gun.burstRemain;

				// プレハブ未設定ならスキップ
				if (_gun.bulletPrefabGUID == Engine::DefaultGUID) continue;

				// プレハブのハンドルを解決(未ロードならロード)
				auto& _rm = *a_ctx.pServices->pResourceManager;
				if (!_rm.IsValid(_gun.bulletPrefabHandle))
				{
					if (!_rm.Has<Engine::Resource::Prefab>(_gun.bulletPrefabGUID))
					{
						_rm.Load<Engine::Resource::Prefab>(_gun.bulletPrefabGUID);
					}
					_gun.bulletPrefabHandle = _rm.GetCache<Engine::Resource::Prefab>(_gun.bulletPrefabGUID);
				}
				auto* _pPrefab = _rm.Ref(_gun.bulletPrefabHandle);
				if (!_pPrefab) continue;

				// ---- 銃の位置と、銃自身のローカル +Z 軸 ----
				const DirectX::XMFLOAT4X4& _m = _worldMat.worldMat;
				DXSM::Vector3 _pos = { _m._41, _m._42, _m._43 };	// 平行移動
				DXSM::Vector3 _gunFwd = { _m._31, _m._32, _m._33 };	// ローカル +Z 軸

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

				DXSM::Vector3 _baseDir = _gunFwd;
				if (_pAim)
				{
					DXSM::Vector3 _aimDir = DXSM::Vector3(_pAim->dir);
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
				DXSM::Vector3 _spawnPos = _pos;
				if (_gun.nullPtrNodeHash != 0)
				{
					auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
					if (_pModel)
					{
						const auto& _nodeVec = _pModel->GetOriginalNodeVec();
						if (_gun.nodeIndex < _nodeVec.size())
						{
							const DirectX::XMFLOAT4X4& _nodeMat = _nodeVec[_gun.nodeIndex].worldTransform;
							DXSM::Vector3 _nodeLocalPos = { _nodeMat._41, _nodeMat._42, _nodeMat._43 };
							_spawnPos = DXSM::Vector3::Transform(_nodeLocalPos, DXSM::Matrix(_m));
						}
					}
				}

				//======================================================================
				// 射出方向 : 銃口から狙点へ向ける
				//======================================================================
				DXSM::Vector3 _shootDir = _baseDir;
				if (_pAim)
				{
					DXSM::Vector3 _toTarget = DXSM::Vector3(_pAim->pos) - _spawnPos;

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

				DirectX::XMFLOAT3 _velValue = _shootDir * _gun.speed;

				// ---- プレハブのデータをコピーして、位置と速度を上書き ----
				Engine::ECS::Signature _sig = _pPrefab->GetSignature();
				auto _data = _pPrefab->GetDataMap();	// コピー(型ID -> バイト列)

				auto _ltID = a_ctx.pWorld->GetCompTypeID<LocalTransformComponent>();
				auto _velID = a_ctx.pWorld->GetCompTypeID<VelocityComponent>();
				auto _wmID = a_ctx.pWorld->GetCompTypeID<WorldMatrixComponent>();

				// 弾が動く・描画されるために最低限必要なコンポーネントが無ければ足す
				auto _ensure = [&](Engine::ECS::ComponentTypeID _id)
				{
					if (_sig.test(_id)) return;
					_sig.set(_id);
					auto& _buf = _data[_id];
					_buf.assign(a_ctx.pWorld->GetComponentMetaData(_id).compAlignSize, 0);
					auto _ctor = a_ctx.pWorld->GetCompFunc(_id).construct;
					if (_ctor) _ctor(_buf.data());
				};
				_ensure(_ltID);
				_ensure(_velID);
				_ensure(_wmID);

				// 位置の上書き
				{
					auto& _buf = _data[_ltID];
					LocalTransformComponent _lt = {};
					std::memcpy(&_lt, _buf.data(), sizeof(_lt));
					_lt.pos = _spawnPos;
					_lt.isDirty = true;
					std::memcpy(_buf.data(), &_lt, sizeof(_lt));
				}
				// 速度の上書き
				{
					auto& _buf = _data[_velID];
					VelocityComponent _v = {};
					std::memcpy(&_v, _buf.data(), sizeof(_v));
					_v.value = _velValue;
					std::memcpy(_buf.data(), &_v, sizeof(_v));
				}
				// 発射元を入れる。弾が自分を撃った相手に当たらないようにするため
				// (銃口は体の中にあるので、入れないと発射した瞬間に自分へ当たる)
				{
					auto _projID = a_ctx.pWorld->GetCompTypeID<ProjectileComponent>();
					auto _it = _data.find(_projID);
					if (_sig.test(_projID) &&
						_it != _data.end() && _it->second.size() >= sizeof(ProjectileComponent))
					{
						ProjectileComponent _proj = {};
						std::memcpy(&_proj, _it->second.data(), sizeof(_proj));
						_proj.shooterEntity = ResolveShooterEntity(*a_ctx.pWorld, _self);
						std::memcpy(_it->second.data(), &_proj, sizeof(_proj));
					}
				}
				// 誘導弾なら追う相手を入れる(持っていない弾には足さない)
				{
					auto _homingID = a_ctx.pWorld->GetCompTypeID<HomingComponent>();
					auto _it = _data.find(_homingID);
					if (_sig.test(_homingID) &&
						_it != _data.end() && _it->second.size() >= sizeof(HomingComponent))
					{
						HomingComponent _homing = {};
						std::memcpy(&_homing, _it->second.data(), sizeof(_homing));
						_homing.targetEntity = ResolveHomingTarget(*a_ctx.pWorld, _self, _pAim);
						std::memcpy(_it->second.data(), &_homing, sizeof(_homing));
					}
				}

				// 反復中なので即時生成せず、遅延生成コマンドに積む
				a_ctx.pWorld->AddEntityWithData(_sig, std::move(_data));
			}
		}
	);
}
