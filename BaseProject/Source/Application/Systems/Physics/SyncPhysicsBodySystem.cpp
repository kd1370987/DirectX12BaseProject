#include "SyncPhysicsBodySystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Physics/Collider.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Application/Systems/Transform/HierarchyTransform.h"

#include "Engine/Physics/PhysicsWorld.h"
#include "Application/Components/Transform/HierarchyComponent.h"

void SyncPhysicsBodySystem::Init(App::ECS::APPWorld& a_world)
{
	// 対象は動的レイヤーのコライダー(+ モデル + トランスフォーム)。
	// ボイドの群れで数千体あり、1体ずつ別々のボディを動かすだけ(PhysicsWorld::SetBodyTransform は
	// ボディごとにロックを取る)なので、チャンクを分けてワーカーで回す
	a_world.ActiveJobTask<const ColliderComponent, const ModelComponent, const LocalTransformComponent>(
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
	)
	// 物理空間のボディを動かす。BallisticSystem のレイ(静的な地面だけを見る)は動かす前に撃たせる。
	// あちらは LocalTransform を書いて物理空間を読み、こちらはその逆なので、向きを決めないと循環する
	.WritesResource<Engine::Physics::PhysicsWorld>()
	.After("BallisticSystem")
	// 絞り込みに使わない読み : 親を辿ってワールド行列を組む(HierarchyTransform)
	.Reads<HierarchyComponent>();
}
