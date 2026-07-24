#include "HierarchyLinkSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Persistence/GUIDComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"

void HierarchyLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.AwakeTask<const GUIDComponent, HierarchyComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"HierarchyLinkSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			AwakeTag* a_tag,
			const GUIDComponent* a_guidArray,
			HierarchyComponent* a_hierarchyArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HierarchyComponent& _hieComp = a_hierarchyArray[_i];

				// ターゲットGUIDがあるのなら
				if (_hieComp.parentGUID != Engine::DefaultGUID)
				{
					_hieComp.parentID = a_ctx.pWorld->GetEntity(_hieComp.parentGUID);
				}
			}
		}
	);
}
