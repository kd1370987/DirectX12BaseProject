#include "GUIDFixupSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Persistence/GUIDComponent.h"

void GUIDFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<GUIDComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"GUIDFixupSystem",
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			PostDeserializeTag* a_tag,
			GUIDComponent* a_guidArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				GUIDComponent& _guidComp = a_guidArray[_i];

				if (_guidComp.guid == Engine::DefaultGUID)
				{
					auto _func = a_world.GetCompFunc<GUIDComponent>();
					_func.construct(&_guidComp);
					// GUIDが付与されていなければ付与
					_guidComp.guid.Create();
				}
			}
		}
	);
}
