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
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag*,
			ColliderComponent* a_collArray,
			const ModelComponent* a_modelArray,
			const LocalTransformComponent*		// 行列は親を辿って組むのでここでは使わない
			)
		{
			ENGINE_PROFILE_SCOPE("Physics_RegisterStatic");

			auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();
			const auto& _resourceManager = *a_ctx.pServices->pResourceManager;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ColliderComponent& _collComp = a_collArray[_i];
				const Engine::ECS::Entity _entity = a_pChunk->entityData[_i];

				// 動くものは Phase 4(常駐ボディ化)で扱う。ここは静的だけ
				if (IsDynamicLayer(_collComp.layer)) continue;

				// 静的で Mesh 以外の形状はアセット上に無い(2026-09-19 時点で0件)。
				// 旧実装は描画メッシュのAABBで概算していたが、こちらでは作らずに知らせる
				if (_collComp.shapeType.type != Engine::Collision::EShapeType::Mesh)
				{
					ENGINE_WARNING("[Physics] 静的コライダーの形状が Mesh ではないので登録しません(entity=%llu)", _entity);
					continue;
				}

				// 作り直し(Release → PostDeserialize → Start)で戻ってきたときに備えて、
				// 前のボディが残っていれば消してから作る(持ち主が違えば何もしない)
				_physicsWorld.DestroyBody(_collComp.physicsBody, _entity);

				Engine::Physics::StaticModelBodyDesc _desc;
				_desc.owner = _entity;
				_desc.modelHandle = a_modelArray[_i].handle;

				// ワールド行列は親を辿って組む。
				// Start の時点では WorldMatrixComponent がまだ空なので使えない
				_desc.worldMat = App::Systems::HierarchyTransform::CalcWorldMatrix(*a_ctx.pWorld, _entity);

				_desc.group = static_cast<uint32_t>(_collComp.layer);
				_desc.mask = static_cast<uint32_t>(_collComp.collideLayer);

				_collComp.physicsBody = _physicsWorld.CreateStaticModelBody(_resourceManager, _desc);
			}
		});
}
