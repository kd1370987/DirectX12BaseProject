#include "HealthSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/HealthComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/InstanceResource/HitEventResource.h"
#include "Application/InstanceResource/DeathEventResource.h"

//==============================================================================
// HealthSystem
//
// そのフレームに受けたヒットぶん体力を減らし、0 になったら自分を消す。
//
// ・ヒットは HitEventResource(ワールドに1つ)を見る。
//   弾は当たった直後に自分が消えるので、当てた側から殴りに行くのではなく
//   「そのフレームに起きたヒット全部」が残るこちらを受け手側が読む。
//   1フレームに複数発当たった場合もすべて食らう(CollisionEvent は1件しか持てない)。
// ・撃破時はエフェクトを出さず、死亡を DeathEventResource へ積むだけにする。
//   何を出すかは DeathEffectComponent、出すのは DeathEffectSystem の仕事で、
//   弾が着弾で消えるときと同じ入口にそろえてある。
// ・体力を持つものは ExplodeOnHitSystem の対象から外してある(Exclude<HealthComponent>)。
//   即死させる役目とここが二重に効かないようにするためで、
//   体力持ちの死亡はこのシステムだけが決める。
// ・削除は AddReleaseEntity で予約する。反復中に消すとチャンクが壊れるため。
//   ReleaseTag 経由にしているのは、敵が持っているポーズ行列やアニメーション用頂点を
//   Release フェーズで返してから消すため(直接消すと返す機会がないまま漏れる)。
// ・爆発の位置に WorldMatrix ではなく LocalTransform を使っているのは、
//   PostUpdate 帯で WorldMatrix を読む ActiveTask を作るとシステムのソートが循環するため
//   (CommitHierarchyWorldMatrixSystem が ActiveTag を読んで WorldMatrix を書いている)。
//   体力を持つのは敵やプレイヤーのような親を持たないエンティティなので、
//   ローカル座標がそのままワールド座標になる。
// ・PostUpdate 帯。ヒットを積むのは Physics 帯の HitDetectSystem、
//   消すのは次フレーム PreUpdate の HitEventClearSystem なので、その間で読む。
//==============================================================================
void HealthSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<HealthComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"HealthSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			HealthComponent*                  a_healthArray,
			const LocalTransformComponent*    a_trsArray
		)
		{
			if (!a_ctx.pWorld->HasResource<HitEventResource>()) return;
			const HitEventResource& _hitEvents = a_ctx.pWorld->GetResource<HitEventResource>();

			// ヒットが1件も無いフレームは何もしない
			if (_hitEvents.events.empty()) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HealthComponent&               _health = a_healthArray[_i];
				const LocalTransformComponent& _trs    = a_trsArray[_i];

				// 既に力尽きているものは二重に処理しない
				if (_health.currentHealth <= 0.0f) continue;

				Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// ---- 自分が受け手になっているヒットを全部食らう ----
				float _damage = 0.0f;
				for (const HitEvent& _event : _hitEvents.events)
				{
					if (_event.victim != _self) continue;
					_damage += _event.damage;
				}

				if (_damage <= 0.0f) continue;

				_health.currentHealth -= _damage;
				if (_health.currentHealth > 0.0f) continue;

				// ---- 撃破 ----
				_health.currentHealth = 0.0f;

				a_ctx.pWorld->AddReleaseEntity(_self);

				// 死亡を積む(エフェクトは DeathEffectSystem が出す)
				if (a_ctx.pWorld->HasResource<DeathEventResource>())
				{
					DeathEvent _death = {};
					_death.entity = _self;
					_death.pos    = _trs.pos;

					a_ctx.pWorld->GetResource<DeathEventResource>().Push(_death);
				}
			}
		}
	);
}
