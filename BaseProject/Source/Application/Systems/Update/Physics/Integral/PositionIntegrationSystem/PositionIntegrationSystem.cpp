#include "PositionIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Transform/TransformComponent.h"

#include "Application/Components/Force/InertiaComponent.h"

void PositionIntegrationSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEachEx<VelocityComponent, TransformComponent>
		(
			[&a_world, a_dt]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				VelocityComponent* a_velocityArray,
				TransformComponent* a_trsArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					VelocityComponent& _velComp = a_velocityArray[_i];
					TransformComponent& _trsComp = a_trsArray[_i];
					_trsComp.pos.x += _velComp.value.x * a_dt;
					_trsComp.pos.y += _velComp.value.y * a_dt;
					_trsComp.pos.z += _velComp.value.z * a_dt;

					_velComp.value = {};
				}
			},
			Engine::ECS::Exclude<InertiaComponent>()
		);
}
