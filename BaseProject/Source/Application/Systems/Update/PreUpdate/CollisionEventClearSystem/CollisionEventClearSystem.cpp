#include "CollisionEventClearSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/ECS/Internal/CollisionEvent.h"

void CollisionEventClearSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<Engine::ECS::CollisionEvent>(
		Engine::ECS::ESystemType::PreUpdate,
		"CollisionEventClearSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			Engine::ECS::CollisionEvent* a_eventArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				a_eventArray[_i].other = Engine::ECS::Limits::INVALID_ENTITY;
			}
		}
	);
}
