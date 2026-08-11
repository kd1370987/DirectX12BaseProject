#include "HealthFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Character/HealthComponent.h"

//==============================================================================
// HealthFixupSystem
//
// 現在体力は保存しないランタイム値なので、生成された時点で最大体力に満たす。
// シーンから読み込まれたエンティティも、プレハブから撃ち出されたエンティティも
// 必ず PostDeserialize を通るので、ここで初期化すれば取りこぼしがない。
//==============================================================================
void HealthFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<HealthComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"HealthFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			HealthComponent* a_healthArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HealthComponent& _health = a_healthArray[_i];

				if (_health.maxHealth < 0.0f) _health.maxHealth = 0.0f;
				_health.currentHealth = _health.maxHealth;
			}
		}
	);
}
