#include "VelocityIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"

void VelocityIntegrationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			VelocityComponent* a_velocityArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{

			}
		}
	);
}

