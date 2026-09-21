#include "SyncPhysicsBodySystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Collision/Collider.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

#include "../../../Shared/HierarchyTransform/HierarchyTransform.h"

#include "Engine/Physics/PhysicsWorld.h"

void SyncPhysicsBodySystem::Init(App::ECS::APPWorld& a_world)
{
	// 対象は動的レイヤーのコライダー(+ モデル + トランスフォーム)
	a_world.ActiveTask<const ColliderComponent, const ModelComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"SyncPhysicsBodySystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*,
			const ColliderComponent* a_collArray,
			const ModelComponent*,
			const LocalTransformComponent*		// 行列は親を辿って組むのでここでは使わない
			)
		{
			ENGINE_PROFILE_SCOPE("Physics_SyncDynamic");
			auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ColliderComponent& _collComp = a_collArray[_i];

				// 静的なボディは Start で置いたまま動かさない
				if (!IsDynamicLayer(_collComp.layer)) continue;
				if (!_collComp.physicsBody.IsValid()) continue;

				// ワールド行列は親を辿って組む(登録と同じ)
				const Engine::ECS::Entity _entity = a_pChunk->entityData[_i];
				const Math::Matrix _mat = App::Systems::HierarchyTransform::CalcWorldMatrix(*a_ctx.pWorld, _entity);

				_physicsWorld.SetBodyTransform(_collComp.physicsBody, _entity, _mat);
			}
		}
	);
}
