#include "InertiaIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/InertiaComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

void InertiaIntegrationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const VelocityComponent, InertiaComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"InertiaIntegrationSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const VelocityComponent* a_velocityArray,
			InertiaComponent* a_inertiaArray,
			LocalTransformComponent* a_trsArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const VelocityComponent& _velComp = a_velocityArray[_i];
				InertiaComponent& _inertiaComp = a_inertiaArray[_i];
				LocalTransformComponent& _trsComp = a_trsArray[_i];

				// 目標速度(VelocityComponent)へ指数的に追従させる
				// value は追従にかかる時定数なので、フレームレートに依存しない
				// 0.0f のときは慣性なしとして目標速度をそのまま使う
				float _rate = 1.0f;
				if (_inertiaComp.value > 0.0001f)
				{
					_rate = 1.0f - std::exp(-a_ctx.dt / _inertiaComp.value);
				}

				_inertiaComp.velocity.x += (_velComp.value.x - _inertiaComp.velocity.x) * _rate;
				_inertiaComp.velocity.y += (_velComp.value.y - _inertiaComp.velocity.y) * _rate;
				_inertiaComp.velocity.z += (_velComp.value.z - _inertiaComp.velocity.z) * _rate;

				if (std::abs(_inertiaComp.velocity.x) > 0.0001f ||
					std::abs(_inertiaComp.velocity.y) > 0.0001f ||
					std::abs(_inertiaComp.velocity.z) > 0.0001f)
				{
					_trsComp.pos.x += _inertiaComp.velocity.x * a_ctx.dt;
					_trsComp.pos.y += _inertiaComp.velocity.y * a_ctx.dt;
					_trsComp.pos.z += _inertiaComp.velocity.z * a_ctx.dt;

					// 座標が変わったのでDirtyフラグを立てる
					_trsComp.isDirty = true;
				}
				else
				{
					// 微小な速度は残さずに止める
					_inertiaComp.velocity = {};
				}
			}
		}
	);
}
