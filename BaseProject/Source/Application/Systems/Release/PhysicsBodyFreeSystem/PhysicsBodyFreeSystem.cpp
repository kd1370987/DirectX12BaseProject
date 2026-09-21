#include "PhysicsBodyFreeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../Components/Collision/Collider.h"

#include "Engine/Physics/PhysicsWorld.h"

void PhysicsBodyFreeSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ReleaseTask<ColliderComponent>(
		Engine::ECS::ESystemType::Release,
		"PhysicsBodyFreeSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag*,
			ColliderComponent* a_collArray
			)
		{
			auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ColliderComponent& _collComp = a_collArray[_i];
				if (!_collComp.physicsBody.IsValid()) continue;

				// 持ち主を照合してから消す(中身ごと複製された札で他人のボディを消さない)
				_physicsWorld.DestroyBody(_collComp.physicsBody, a_pChunk->entityData[_i]);
				_collComp.physicsBody = {};
			}
		}
	);
}
