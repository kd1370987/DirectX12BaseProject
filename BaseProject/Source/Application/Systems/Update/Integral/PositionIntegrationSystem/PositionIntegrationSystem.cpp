#include "PositionIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Transform/TRSComponent.h"

#include "../../../../Components/Force/InertiaComponent.h"

void PositionIntegrationSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEachEx<VelocityComponent, TRSComponent>
		(
			[&a_world, a_dt]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				VelocityComponent* a_velocityArray,
				TRSComponent* a_trsArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					VelocityComponent& _velComp = a_velocityArray[_i];
					TRSComponent& _trsComp = a_trsArray[_i];
					_trsComp.pos.x += _velComp.value.x * a_dt;
					_trsComp.pos.y += _velComp.value.y * a_dt;
					_trsComp.pos.z += _velComp.value.z * a_dt;

					_velComp.value = {};
				}
			},
			Exclude<InertiaComponent>()
		);
}
