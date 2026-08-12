#include "ExplodeOnHitSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/ECS/Internal/CollisionEvent.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Collision/ExplodeOnHitComponent.h"
#include "Application/Components/Character/HealthComponent.h"
#include "Application/InstanceResource/DeathEventResource.h"

//==============================================================================
// ExplodeOnHitSystem
//
// 何かに当たった瞬間に消えるもの(弾・ミサイルなど)を処理する。
//
// エフェクトはここでは出さない。当たった位置を「死亡」として積むだけで、
// 何を出すかは DeathEffectComponent、出すのは DeathEffectSystem が持つ
// (体力切れの死と同じ入口にそろえてある)。
//
// 体力を持つもの(HealthComponent)はここでは扱わない。
// 敵のように「殴られても耐える」ものまで一撃で消えてしまうため、
// 体力持ちの死亡は HealthSystem 側に一本化している。
//==============================================================================
void ExplodeOnHitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const Engine::ECS::CollisionEvent, ExplodeOnHitComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"ExplodeOnHitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const Engine::ECS::CollisionEvent* a_eventArray,
			ExplodeOnHitComponent* a_explodeArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const Engine::ECS::CollisionEvent& _event = a_eventArray[_i];
				ExplodeOnHitComponent& _explode = a_explodeArray[_i];

				// 未ヒットならスキップ
				if (_event.other == Engine::ECS::Limits::INVALID_ENTITY) continue;

				// ---- 自分を消す ----
				if (!_explode.destroySelf) continue;

				Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// 解放処理を通してから消す(飛翔音のボイスなど借りているものを返すため)
				a_ctx.pWorld->AddReleaseEntity(_self);

				// ---- 死亡を積む(エフェクトは DeathEffectSystem が出す) ----
				// 弾の場合は自分の位置ではなく当たった位置に出したいので hitPos を渡す
				if (a_ctx.pWorld->HasResource<DeathEventResource>())
				{
					DeathEvent _death = {};
					_death.entity = _self;
					_death.pos    = _event.hitPos;

					a_ctx.pWorld->GetResource<DeathEventResource>().Push(_death);
				}
			}
		},
		// 体力を持つものは HealthSystem が面倒を見る
		Engine::ECS::Exclude<HealthComponent>()
	);
}
