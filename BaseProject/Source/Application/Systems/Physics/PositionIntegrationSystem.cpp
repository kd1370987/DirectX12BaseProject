#include "PositionIntegrationSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Movement/DesiredVelocityComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Application/Components/Movement/MovementParamsComponent.h"

namespace App::System
{
	void PositionIntegrationSystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクの配列だけを書くので、チャンクを分けてワーカーで回す。
		// MovementIntegrationSystem とは対象のアーキタイプが重ならないので、同時に走る
		a_world.ActiveJobTask<const Component::DesiredVelocityComponent, Component::LocalTransformComponent>(
			Engine::ECS::ESystemType::Physics,
			"PositionIntegrationSystem",
			[](
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* a_tags,
				const Component::DesiredVelocityComponent* a_velocityArray,
				Component::LocalTransformComponent* a_trsArray
			) 
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::DesiredVelocityComponent& _velComp = a_velocityArray[_i];
					Component::LocalTransformComponent& _trsComp = a_trsArray[_i];

					if (std::abs(_velComp.value.x) > 0.0001f ||
						std::abs(_velComp.value.y) > 0.0001f ||
						std::abs(_velComp.value.z) > 0.0001f)
					{
						Component::LocalTransformComponent& _trsComp = a_trsArray[_i];

						_trsComp.pos.x += _velComp.value.x * a_ctx.dt;
						_trsComp.pos.y += _velComp.value.y * a_ctx.dt;
						_trsComp.pos.z += _velComp.value.z * a_ctx.dt;

						// 座標が変わったのでDirtyフラグを立てる
						_trsComp.isDirty = true;
					}
				}
			},
			// 加減速を持つ側は MovementIntegrationSystem が実速度で進める
			Engine::ECS::Exclude<Component::MovementParamsComponent>()
		);
	}
}