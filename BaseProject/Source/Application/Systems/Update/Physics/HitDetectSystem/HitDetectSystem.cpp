#include "HitDetectSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/ECS/Internal/CollisionEvent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"

#include "Application/Components/Collision/SphereCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

void HitDetectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const SphereColliderComponent, Engine::ECS::CollisionEvent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"HitDetectSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const SphereColliderComponent* a_sphereArray,
			Engine::ECS::CollisionEvent* a_eventArray,
			const LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = a_ctx.pServices->pMainEngine->RefCollisionWorld();
			if (!_pCollWorld) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const SphereColliderComponent& _sphere = a_sphereArray[_i];
				const LocalTransformComponent& _trans = a_transArray[_i];
				Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// 自分の球で重なりクエリ(自分自身は除外)
				Engine::Collision::SphereInfo _info;
				_info.origin = DXSM::Vector3(_trans.pos) + DXSM::Vector3(_sphere.offset);
				_info.radius = _sphere.radius;

				Engine::Collision::Result _res = {};
				if (!_pCollWorld->VsSphere(_info, _res, _self)) continue;
				if (!_res.isHit) continue;

				// 自分側に記録(弾が hitPos で反応/消滅するため)
				a_eventArray[_i].other  = _res.hitEntity;
				a_eventArray[_i].hitPos = _res.hitPos;
				a_eventArray[_i].hitDir = _res.hitNormal;

				// 当たった相手側にも直接書き込む(相手が CollisionEvent を持っていれば)。
				// 値の書き換えのみ＝構造変化なしなので反復中でも安全。
				if (_res.hitEntity != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->HasComponent<Engine::ECS::CollisionEvent>(_res.hitEntity))
				{
					auto* _ev = a_ctx.pWorld->RefData<Engine::ECS::CollisionEvent>(_res.hitEntity);
					if (_ev)
					{
						_ev->other  = _self;
						_ev->hitPos = _res.hitPos;
						// 相手から見た方向は逆
						_ev->hitDir = { -_res.hitNormal.x, -_res.hitNormal.y, -_res.hitNormal.z };
					}
				}
			}
		}
	);
}
