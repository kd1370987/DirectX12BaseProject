#include "GUIDFixupSystem.h"
#include "Application/ECS/World/APPWorld.h"

#include "Engine/ECS/Component/GUIDComponent.h"

namespace App::System
{
	void GUIDFixupSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.PostDeserializeTask<Engine::ECS::GUIDComponent>(
			Engine::ECS::ESystemType::PostDeserialize,
			"GUIDFixupSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::PostDeserializeTag* /*a_tag*/,
				Engine::ECS::GUIDComponent* a_guidArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Engine::ECS::GUIDComponent& _guidComp = a_guidArray[_i];

					if (_guidComp.guid == Core::DEFAULT_GUID)
					{
						auto _func = a_ctx.pWorld->GetCompFunc<Engine::ECS::GUIDComponent>();
						_func.construct(&_guidComp);
						// GUIDが付与されていなければ付与
						_guidComp.guid.Create();
					}
				}
			}
		);
	}
}
