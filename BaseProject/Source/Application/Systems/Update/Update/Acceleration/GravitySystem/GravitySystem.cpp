#include "GravitySystem.h"

#include "Engine/ECS/World/World.h"
#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/GravityComponent.h"

void GravitySystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const GravityComponent, VelocityComponent>(
		Engine::ECS::ESystemType::Physics,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt, 
			ActiveTag* a_tags,
			const GravityComponent* a_gravityArray,
			VelocityComponent* a_velocityArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const GravityComponent& _gravComp = a_gravityArray[_i];
				VelocityComponent& _velComp = a_velocityArray[_i];
				_velComp.value.y += _gravComp.scale;
			}
		}
	);
}
