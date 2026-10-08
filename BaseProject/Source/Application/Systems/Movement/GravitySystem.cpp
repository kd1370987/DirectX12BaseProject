#include "GravitySystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"
#include "Application/Components/Movement/GravityComponent.h"

namespace App::System
{
	void GravitySystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクの配列だけを書くので、チャンクを分けてワーカーで回す
		a_world.ActiveJobTask<const Component::GravityComponent, Component::DesiredVelocityComponent>(
			Engine::ECS::ESystemType::Physics,
			"GravitySystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& /*a_ctx*/, 
				Component::ActiveTag* /*a_tags*/,
				const Component::GravityComponent* a_gravityArray,
				Component::DesiredVelocityComponent* a_velocityArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::GravityComponent& _gravComp = a_gravityArray[_i];
					Component::DesiredVelocityComponent& _velComp = a_velocityArray[_i];
					_velComp.value.y -= _gravComp.scale;
				}
			}
		);
	}
}
