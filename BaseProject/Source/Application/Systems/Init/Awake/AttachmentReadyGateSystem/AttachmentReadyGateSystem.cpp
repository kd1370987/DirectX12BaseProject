#include "AttachmentReadyGateSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Hierarchy/FollowAnimationNodeComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../InstanceResource/ResourceWaitResource.h"

void AttachmentReadyGateSystem::Init(App::ECS::World& a_world)
{
	a_world.AwakeTask<const FollowAnimationNodeComponent, const HierarchyComponent>(
		Engine::ECS::ESystemType::Awake,
		"AttachmentReadyGateSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			AwakeTag* a_awakeTag,
			const FollowAnimationNodeComponent* a_pFollowArray,
			const HierarchyComponent* a_pHierarchyArray
		)
		{
			auto& _wait = a_ctx.pWorld->GetResource<ResourceWaitResource>();
			auto& _resMgr = *a_ctx.pServices->pResourceManager;

			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				const HierarchyComponent& _hierarchyComp = a_pHierarchyArray[_i];

				// 親がまだ結びついていない。
				// 親を持つはずなのに解決されていないだけなので、繋がるまで待つ。
				// 親を持たない構成(設定漏れ)で永久に止まらないよう、
				// GUIDが入っているときだけ待つ
				if (_hierarchyComp.parentID == Engine::ECS::Limits::INVALID_ENTITY)
				{
					if (_hierarchyComp.parentGUID.IsValid())
					{
						_wait.AddWait(a_pChunk->entityData[_i]);
					}
					continue;
				}

				// 親がモデルを持たない構成なら、待つものがない
				const auto* _pParentModelComp = a_ctx.pWorld->RefData<ModelComponent>(_hierarchyComp.parentID);
				if (!_pParentModelComp) continue;
				if (_pParentModelComp->modelGUID == Engine::DefaultGUID) continue;

				// 待つのは読込中のときだけ。
				// Failed をここで待たせると、もう届かないものを永久に待つことになる
				const auto _state = _resMgr.GetState(_pParentModelComp->handle);
				if (_state != Engine::Resource::EResourceState::Loading) continue;

				_wait.AddWait(a_pChunk->entityData[_i]);
			}
		}
	);
}
