#include "VelocityIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Force/VelocityComponent.h"

void VelocityIntegrationSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<VelocityComponent>
		(
			[&a_world, a_dt]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				VelocityComponent* a_velocityArray
				)
			{
				
			}
		);
}
