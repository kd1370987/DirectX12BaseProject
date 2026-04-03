#include "VelocityIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"

void VelocityIntegrationSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<VelocityComponent>
		(
			[&a_world, a_dt]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				VelocityComponent* a_velocityArray
				)
			{
				
			}
		);
}
