#include "AdditivePoseFreeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Character/Robot/AdditivePoseComponent.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

void AdditivePoseFreeSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ReleaseTask<AdditivePoseComponent>(
		Engine::ECS::ESystemType::Release,
		"AdditivePoseFreeSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag* a_releaseTag,
			AdditivePoseComponent* a_additiveArray
		)
		{
			auto& _entryPool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				AdditivePoseComponent& _addComp = a_additiveArray[_i];

				_entryPool.FreeRange(_addComp.handle);
				_addComp.handle = {};
			}
		}
	);
}
