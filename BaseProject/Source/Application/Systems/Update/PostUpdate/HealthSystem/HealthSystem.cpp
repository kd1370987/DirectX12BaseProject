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
// ・撃破しても ここでは消さない。HealthComponent を「死亡状態」にするだけで、
//   実際に消す(解放予約する)のは releaseDelay 秒あとの DeathStateSystem。
//
//   以前はここで AddReleaseEntity まで済ませていたが、それだと死亡を読む側が
//   1フレームでも遅れると本人がもう居らず、死亡エフェクトが出せなかった。
//   死んだ本人のコンポーネントを引く処理(DeathEffectSystem など)のために、
//   死んでからしばらくは生かしておく。
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

				// ---- 撃破 : 消さずに死亡状態へ入る ----
				_health.currentHealth = 0.0f;
				_health.isDead        = true;
				_health.deathTimer    = 0.0f;

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
