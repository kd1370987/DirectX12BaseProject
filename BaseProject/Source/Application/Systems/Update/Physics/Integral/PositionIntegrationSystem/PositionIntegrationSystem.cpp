#include "PositionIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Transform/TransformComponent.h"

#include "Application/Components/Force/InertiaComponent.h"

void PositionIntegrationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const VelocityComponent, TransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"PositionIntegrationSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const VelocityComponent* a_velocityArray,
			TransformComponent* a_trsArray
		) 
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const VelocityComponent& _velComp = a_velocityArray[_i];
				TransformComponent& _trsComp = a_trsArray[_i];
				_trsComp.pos.x += _velComp.value.x * a_dt;
				_trsComp.pos.y += _velComp.value.y * a_dt;
				_trsComp.pos.z += _velComp.value.z * a_dt;

				//_velComp.value = {}; いったんインプットムーブ側に移している
			}
		},
		Engine::ECS::Exclude<InertiaComponent>()
	);
}