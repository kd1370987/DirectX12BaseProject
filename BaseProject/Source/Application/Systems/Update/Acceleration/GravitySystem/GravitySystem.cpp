#include "GravitySystem.h"

#include "Engine/ECS/World/World.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Force/GravityComponent.h"

void GravitySystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<GravityComponent,VelocityComponent>(
		[&a_world]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			GravityComponent* a_gravityArray,
			VelocityComponent* a_velocityArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				GravityComponent& _gravComp = a_gravityArray[_i];
				VelocityComponent& _velComp = a_velocityArray[_i];
				_velComp.value.y += _gravComp.scale;
			}
		}
	);
}
