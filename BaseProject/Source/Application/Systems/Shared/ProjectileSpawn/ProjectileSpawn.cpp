#include "ProjectileSpawn.h"

#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../../../Components/Transform/LocalTransformComponent.h"
#include "../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../Components/Force/VelocityComponent.h"
#include "../../../Components/Character/Weapon/Projectile/HomingComponent.h"
#include "../../../Components/Character/Weapon/Projectile/ProjectileComponent.h"
#include "../../../Components/Hierarchy/HierarchyComponent.h"
#include "../../../Components/Collision/Collider.h"

namespace App::Systems::ProjectileSpawn
{
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

	void Spawn(
		Engine::ECS::World&       a_world,
		Engine::Resource::Prefab* a_pPrefab,
		const DirectX::XMFLOAT3&  a_pos,
		const DirectX::XMFLOAT3&  a_velocity,
		Engine::ECS::Entity       a_shooter,
		Engine::ECS::Entity       a_homingTarget)
	{
		if (!a_pPrefab) return;

		// ---- プレハブのデータをコピーして、位置と速度を上書き ----
		// 子を持つプレハブ(噴煙エフェクト付きの弾など)でも落とさないよう、
		// 材料の組み立てはプレハブ側に任せる。先頭がルート=弾本体。
		std::vector<Engine::Resource::PrefabInstanceData> _instanceVec =
			a_pPrefab->BuildInstanceData(&a_world);
		if (_instanceVec.empty()) return;

		// 子は保存された姿のまま出す(位置と向きは親に追従する)
		for (size_t _c = 1; _c < _instanceVec.size(); ++_c)
		{
			a_world.AddEntityWithData(
				_instanceVec[_c].sig, std::move(_instanceVec[_c].dataMap));
		}

		Engine::ECS::Signature _sig = _instanceVec[0].sig;
		auto _data = std::move(_instanceVec[0].dataMap);	// (型ID -> バイト列)

		auto _ltID  = a_world.GetCompTypeID<LocalTransformComponent>();
		auto _velID = a_world.GetCompTypeID<VelocityComponent>();
		auto _wmID  = a_world.GetCompTypeID<WorldMatrixComponent>();

		// 弾が動く・描画されるために最低限必要なコンポーネントが無ければ足す
		auto _ensure = [&](Engine::ECS::ComponentTypeID _id)
		{
			if (_sig.test(_id)) return;
			_sig.set(_id);
			auto& _buf = _data[_id];
			_buf.assign(a_world.GetComponentMetaData(_id).compAlignSize, 0);
			auto _ctor = a_world.GetCompFunc(_id).construct;
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
			_lt.pos = a_pos;
			_lt.isDirty = true;
			std::memcpy(_buf.data(), &_lt, sizeof(_lt));
		}
		// 速度の上書き
		{
			auto& _buf = _data[_velID];
			VelocityComponent _v = {};
			std::memcpy(&_v, _buf.data(), sizeof(_v));
			_v.value = a_velocity;
			std::memcpy(_buf.data(), &_v, sizeof(_v));
		}
		// 発射元を入れる。弾が自分を撃った相手に当たらないようにするため
		// (銃口は体の中にあるので、入れないと発射した瞬間に自分へ当たる)
		{
			auto _projID = a_world.GetCompTypeID<ProjectileComponent>();
			auto _it = _data.find(_projID);
			if (_sig.test(_projID) &&
				_it != _data.end() && _it->second.size() >= sizeof(ProjectileComponent))
			{
				ProjectileComponent _proj = {};
				std::memcpy(&_proj, _it->second.data(), sizeof(_proj));
				_proj.shooterEntity = a_shooter;
				std::memcpy(_it->second.data(), &_proj, sizeof(_proj));
			}
		}
		// 誘導弾なら追う相手を入れる(持っていない弾には足さない)
		{
			auto _homingID = a_world.GetCompTypeID<HomingComponent>();
			auto _it = _data.find(_homingID);
			if (_sig.test(_homingID) &&
				_it != _data.end() && _it->second.size() >= sizeof(HomingComponent))
			{
				HomingComponent _homing = {};
				std::memcpy(&_homing, _it->second.data(), sizeof(_homing));
				_homing.targetEntity = a_homingTarget;
				std::memcpy(_it->second.data(), &_homing, sizeof(_homing));
			}
		}

		// 反復中なので即時生成せず、遅延生成コマンドに積む
		a_world.AddEntityWithData(_sig, std::move(_data));
	}
}
