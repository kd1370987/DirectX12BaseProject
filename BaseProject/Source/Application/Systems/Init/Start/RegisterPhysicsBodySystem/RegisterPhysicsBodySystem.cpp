#include "RegisterPhysicsBodySystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Collision/Collider.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

#include "../../../Shared/HierarchyTransform/HierarchyTransform.h"

#include "Engine/Physics/PhysicsWorld.h"

void RegisterPhysicsBodySystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.StartTask<ColliderComponent, const ModelComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Start,
		"RegisterPhysicsBodySystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag*,
			ColliderComponent* a_collArray,
			const ModelComponent* a_modelArray,
			const LocalTransformComponent*		// 行列は親を辿って組むのでここでは使わない
			)
		{
			ENGINE_PROFILE_SCOPE("Physics_RegisterBody");

			auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();
			const auto& _resourceManager = *a_ctx.pServices->pResourceManager;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ColliderComponent& _collComp = a_collArray[_i];
				const Engine::ECS::Entity _entity = a_pChunk->entityData[_i];

				// 作り直し(Release → PostDeserialize → Start)で戻ってきたときに備えて、
				// 前のボディが残っていれば消してから作る(持ち主が違えば何もしない)
				_physicsWorld.DestroyBody(_collComp.physicsBody, _entity);

				Engine::Physics::ModelBodyDesc _desc;
				_desc.owner = _entity;
				_desc.modelHandle = a_modelArray[_i].handle;

				// ワールド行列は親を辿って組む。
				// Start の時点では WorldMatrixComponent がまだ空なので使えない
				_desc.worldMat = App::Systems::HierarchyTransform::CalcWorldMatrix(*a_ctx.pWorld, _entity);

				// 形状 : Mesh は判定メッシュ、それ以外は描画メッシュのAABBの箱。
				// (Sphere/Box/Capsule の寸法はコンポーネントに無い。以前の自作判定もAABBで概算していた)
				_desc.shape = (_collComp.shapeType == Engine::Physics::EShapeType::Mesh)
					? Engine::Physics::EModelBodyShape::CollisionMesh
					: Engine::Physics::EModelBodyShape::DrawBounds;

				// 動くもの(敵・弾・ボイド)は Kinematic で常駐させ、SyncPhysicsBodySystem が毎フレーム位置を合わせる
				_desc.isMoving = IsDynamicLayer(_collComp.layer);

				_desc.group = static_cast<uint32_t>(_collComp.layer);
				_desc.mask = static_cast<uint32_t>(_collComp.collideLayer);

				_collComp.physicsBody = _physicsWorld.CreateModelBody(_resourceManager, _desc);
			}
		});
}
