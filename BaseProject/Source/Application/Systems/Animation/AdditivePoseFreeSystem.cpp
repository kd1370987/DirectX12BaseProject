#include "AdditivePoseFreeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Animation/AdditivePoseComponent.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

namespace App::System
{
	void AdditivePoseFreeSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ReleaseTask<Component::AdditivePoseComponent>(
			Engine::ECS::ESystemType::Release,
			"AdditivePoseFreeSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ReleaseTag* /*a_releaseTag*/,
				Component::AdditivePoseComponent* a_additiveArray
			)
			{
				auto& _entryPool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<InstanceResource::AdditiveBoneEntry>>();

				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Component::AdditivePoseComponent& _addComp = a_additiveArray[_i];

					_entryPool.FreeRange(_addComp.handle);
					_addComp.handle = {};
				}
			}
		);
	}
}
