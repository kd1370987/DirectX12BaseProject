#include "UpdateHierarchyDepthSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Hierarchy/HierarchyComponent.h"
#include "../../../../InstanceResource/HierarchyResource.h"

void UpdateHierarchyDepthSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const HierarchyComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"UpdateHierarchyDepthSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const HierarchyComponent* a_hierarychyArray
		)
		{
			// 参照
			auto& _hRes = a_world.GetResource<HierarchyResource>();
			if (!_hRes.isDirty) return;

			// 階層変更の可能性があるのなら走査して検出する
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const auto& _hComp = a_hierarychyArray[_i];

				// 最大深度が更新されていたら変更
				if (_hComp.depth > _hRes.maxDepth)
				{
					ENGINE_LOG("階層に変更がありました : %d -> %d", _hRes.maxDepth, _hComp.depth);
					_hRes.maxDepth = _hComp.depth;
				}
			}
		}
	);
}
