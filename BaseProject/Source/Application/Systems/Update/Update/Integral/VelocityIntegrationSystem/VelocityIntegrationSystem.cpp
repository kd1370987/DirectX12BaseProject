#include "VelocityIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"

void VelocityIntegrationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.RegisterTask<VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			VelocityComponent* a_velocityArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{

			}
		}
	);
}

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
