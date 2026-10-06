#include "SwarmBurstSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/SwarmBurstComponent.h"
#include "Application/Components/Combat/HealthComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Application/InstanceResource/HitEventResource.h"

namespace App::System
{
	//==============================================================================
	// SwarmBurstSystem
	//
	// ワームボスが死ぬときに爆散したボイド(SwarmBurstComponent)を飛ばし、時間が来たら落とす。
	// 爆散させる(群れの部品を外してこれを付ける)のは SwarmBossController。
	//
	//   飛行(PreUpdate / ワーカー)
	//     重力と減速を掛けた速度を目標速度(DesiredVelocity)へ書く。座標を進めるのは
	//     MovementIntegrationSystem(爆散させるときに加減速を 0 にしてあるので、目標速度がそのまま乗る)。
	//     HomingSystem と同じ理由で PreUpdate 帯に置く。
	//
	//   落下(Update / メインスレッド)
	//     残り時間が切れたら、自分に体力ぶんのダメージを積んで落とす。死亡状態へ入れるのも
	//     死亡エフェクトを出すのも HealthSystem 側(撃ち落とされたときと同じ流れ)。
	//     ヒットを積むのは PreUpdate のクリアより後・HealthSystem(PostUpdate)より前なので Update 帯。
	//==============================================================================
	void SwarmBurstSystem::Init(App::ECS::APPWorld& a_world)
	{
		//--------------------------------------------------------------------------
		// 飛行 : 自分のチャンクの値だけを書くので、ワーカーで回す
		//--------------------------------------------------------------------------
		a_world.ActiveJobTask<Component::SwarmBurstComponent, Component::DesiredVelocityComponent, const Component::HealthComponent>(
			Engine::ECS::ESystemType::PreUpdate,
			"SwarmBurstFlightSystem",
			[](
				Engine::ECS::Chunk*,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag*,
				Component::SwarmBurstComponent* a_burstArray,
				Component::DesiredVelocityComponent* a_velArray,
				const Component::HealthComponent* a_healthArray
			)
			{
				const float _dt = a_ctx.dt;

				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					Component::SwarmBurstComponent& _burst = a_burstArray[_i];

					// 落ちたものはその場で止める
					if (a_healthArray[_i].isDead || _burst.isExploded)
					{
						a_velArray[_i].value = Math::Vector3(0.0f, 0.0f, 0.0f);
						continue;
					}

					_burst.timer -= _dt;

					_burst.velocity.y -= _burst.gravity * _dt;
					_burst.velocity   *= std::max(1.0f - _burst.drag * _dt, 0.0f);

					a_velArray[_i].value = _burst.velocity;
				}
			}
		);

		//--------------------------------------------------------------------------
		// 落下 : 時間が来たら自分を落とす
		//--------------------------------------------------------------------------
		a_world.ActiveTask<Component::SwarmBurstComponent, const Component::LocalTransformComponent, const Component::HealthComponent>(
			Engine::ECS::ESystemType::Update,
			"SwarmBurstExplodeSystem",
			[](
				Engine::ECS::Chunk*               a_pChunk,
				uint32_t                          a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag*,
				Component::SwarmBurstComponent*              a_burstArray,
				const Component::LocalTransformComponent*    a_trsArray,
				const Component::HealthComponent*            a_healthArray
			)
			{
				auto& _world = *a_ctx.pWorld;
				if (!_world.HasResource<InstanceResource::HitEventResource>()) return;
				InstanceResource::HitEventResource& _hitEvents = _world.RefResource<InstanceResource::HitEventResource>();

				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					Component::SwarmBurstComponent& _burst = a_burstArray[_i];
					const Component::HealthComponent& _health = a_healthArray[_i];
					if (_burst.isExploded || _health.isDead) continue;
					if (_burst.timer > 0.0f) continue;

					// 残りの体力ぶんのダメージを自分に積む(防御比率は爆散させるときに 1 へ戻してある)
					const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

					InstanceResource::HitEvent _event = {};
					_event.attacker = _self;
					_event.victim   = _self;
					_event.hitPos   = a_trsArray[_i].pos;
					_event.hitDir   = _burst.velocity;
					_event.damage   = std::max(_health.currentHealth, 0.0f) + 1.0f;
					_event.type     = InstanceResource::EHitEventType::Explosion;
					_hitEvents.Push(_event);

					_burst.isExploded = true;
				}
			}
		)
		.WritesResource<InstanceResource::HitEventResource>();
	}
}
