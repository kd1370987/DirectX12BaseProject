#include "EffectEventSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Effect/EffectEventsComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/InstanceResource/DeathEventResource.h"
#include "Application/InstanceResource/HitEventResource.h"
#include "Application/Utility/EffectSpawnHelper.h"

//==========================================================================================
// EffectEventSystem
//
// EffectEventsComponent(出来事 → エフェクトの対応表)を受け持つ。
//
//   Fixup   : GUID からハンドルを解決し、パーティクルのプールを先に作っておく(Warmup)。
//             死んだ瞬間・当たった瞬間に読み込みが走って、最初の一発が遅れないようにするため
//   OnSpawn : Active になった最初のフレームに、自分の位置へ出す
//   OnDeath : DeathEventResource に積まれた死亡について、死んだ位置へ出す
//   OnHit   : HitEventResource の victim について、当たった位置へ出す
//
// ・出すのは SpawnEffectAt(遅延生成。実体化は次の BeginFrame)。出したものは出し切ったら自分から消える。
// ・死亡と被弾は、出来事を積む側(HealthSystem / ExplodeOnHitSystem / HitDetectSystem)が
//   エフェクトの存在を知らなくて済むよう、リソース経由で受け取る。
//   読むと宣言しておかないと、積まれる前に読んで毎フレーム空振りする。
//==========================================================================================
namespace
{
	// 対応表から、指定の出来事の行を全部出す
	void SpawnEventEffects(
		Engine::ECS::World& a_world,
		const EffectEventsComponent& a_comp,
		EEffectEvent a_event,
		const Math::Vector3& a_pos)
	{
		for (const EffectEventEntry& _entry : a_comp.entries)
		{
			if (_entry.event != a_event || !_entry.IsValid()) continue;

			if (!App::Utility::SpawnEffectAt(a_world, _entry.effectGUID, a_pos, true, {}, _entry.scale))
			{
				ENGINE_LOG("[EffectEvent] エフェクトを出せなかった : %s", _entry.effectGUID.String().c_str());
			}
		}
	}
}

void EffectEventSystem::Init(App::ECS::APPWorld& a_world)
{
	//------------------------------------------------------------------------------------------
	// Fixup : ハンドルの解決と先読み
	//
	// プレハブから写した値は持ち主ではないので、必ず取り直す。
	// OnSpawn の印もここで下ろす(入り直したときにもう一度出す)
	//------------------------------------------------------------------------------------------
	a_world.PostDeserializeTask<EffectEventsComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"EffectEventFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			EffectEventsComponent* a_eventsArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				EffectEventsComponent& _comp = a_eventsArray[_i];
				_comp.isSpawnFired = false;

				for (EffectEventEntry& _entry : _comp.entries)
				{
					if (!_entry.IsValid())
					{
						_entry.effectHandle = {};
						continue;
					}

					_pResourceManager->AcquireImmediate(_entry.effectHandle, _entry.effectGUID);
					App::Utility::WarmupEffectParticles(*a_ctx.pServices, _entry.effectHandle);
				}
			}
		}
	);

	//------------------------------------------------------------------------------------------
	// OnSpawn : Active になった最初のフレームに、自分の位置へ出す
	//
	// Start フェーズではワールド行列がまだ計算されていない(CalcMatrix は PostUpdate)ので、
	// PostUpdate でワールド行列を読んでから出す(読むと宣言しているので CalcMatrix の後ろに並ぶ)
	//------------------------------------------------------------------------------------------
	a_world.ActiveTask<EffectEventsComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"EffectEventSpawnSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			EffectEventsComponent* a_eventsArray,
			const WorldMatrixComponent* a_worldMatArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				EffectEventsComponent& _comp = a_eventsArray[_i];
				if (_comp.isSpawnFired) continue;
				_comp.isSpawnFired = true;

				const Math::Matrix _world(a_worldMatArray[_i].worldMat);
				SpawnEventEffects(*a_ctx.pWorld, _comp, EEffectEvent::OnSpawn, _world.Translation());
			}
		}
	);

	//------------------------------------------------------------------------------------------
	// OnDeath : 死んだ位置へ
	//
	// 1フレームに1回だけ走らせたいのでカスタムタスク。
	// 死んだ本人はこの時点ではまだ居るので、コンポーネントを引ける
	//------------------------------------------------------------------------------------------
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		"EffectEventDeathSystem",
		Engine::ECS::ReadList<EffectEventsComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<DeathEventResource>()) return;

			const auto& _deathEvents = a_ctx.pWorld->GetResource<DeathEventResource>();
			for (const DeathEvent& _event : _deathEvents.events)
			{
				if (_event.entity == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!a_ctx.pWorld->HasComponent<EffectEventsComponent>(_event.entity)) continue;

				const auto* _pComp = a_ctx.pWorld->RefData<EffectEventsComponent>(_event.entity);
				if (!_pComp) continue;

				SpawnEventEffects(*a_ctx.pWorld, *_pComp, EEffectEvent::OnDeath, _event.pos);
			}
		}
	)
	// 死亡の一覧を読む(積むのは HealthSystem / ExplodeOnHitSystem)
	.ReadsResource<DeathEventResource>();

	//------------------------------------------------------------------------------------------
	// OnHit : 当たった位置へ
	//
	// 当てた側(弾)は当たった次の瞬間に消えるので、受けた側(victim)の表を見る。
	// 連射を受けている間の鳴らしすぎは、音の側(AudioManager の最短間隔)で間引く
	//------------------------------------------------------------------------------------------
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		"EffectEventHitSystem",
		Engine::ECS::ReadList<EffectEventsComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<HitEventResource>()) return;

			const auto& _hitEvents = a_ctx.pWorld->GetResource<HitEventResource>();
			for (const HitEvent& _event : _hitEvents.events)
			{
				if (_event.victim == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!a_ctx.pWorld->HasComponent<EffectEventsComponent>(_event.victim)) continue;

				const auto* _pComp = a_ctx.pWorld->RefData<EffectEventsComponent>(_event.victim);
				if (!_pComp) continue;

				SpawnEventEffects(*a_ctx.pWorld, *_pComp, EEffectEvent::OnHit, _event.hitPos);
			}
		}
	)
	// ヒットの一覧を読む(積むのは HitDetectSystem、消すのは次フレームの HitEventClearSystem)
	.ReadsResource<HitEventResource>();
}
