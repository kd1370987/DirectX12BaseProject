#include "PositionIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Application/Components/Force/InertiaComponent.h"

void PositionIntegrationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const VelocityComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"PositionIntegrationSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const VelocityComponent* a_velocityArray,
			LocalTransformComponent* a_trsArray
		) 
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const VelocityComponent& _velComp = a_velocityArray[_i];
				LocalTransformComponent& _trsComp = a_trsArray[_i];

				if (std::abs(_velComp.value.x) > 0.0001f ||
					std::abs(_velComp.value.y) > 0.0001f ||
					std::abs(_velComp.value.z) > 0.0001f)
				{
					LocalTransformComponent& _trsComp = a_trsArray[_i];

					_trsComp.pos.x += _velComp.value.x * a_dt;
					_trsComp.pos.y += _velComp.value.y * a_dt;
					_trsComp.pos.z += _velComp.value.z * a_dt;

					// 座標が変わったのでDirtyフラグを立てる
					_trsComp.isDirty = true;
				}
			}
		},
		Engine::ECS::Exclude<InertiaComponent>()
	);
}