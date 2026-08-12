#include "DeathEffectSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/DeathEffectComponent.h"
#include "Application/InstanceResource/DeathEventResource.h"
#include "Application/Utility/PrefabSpawnHelper.h"

//==============================================================================
// DeathEffectSystem
//
// そのフレームに死んだエンティティを見て、DeathEffectComponent に登録されている
// プレハブを死んだ位置へ生成する。エフェクト側の演出(ExplosionComponent)や
// 後片付け(LifeTimeComponent)はプレハブが持つので、ここは出すだけ。
//
// ・死亡は DeathEventResource から受け取る。死因を持つシステム
//   (体力切れ = HealthSystem / 着弾 = ExplodeOnHitSystem)は
//   「誰がどこで死んだか」を積むだけでよく、エフェクトの存在を知らない。
//   死因が増えても積んでもらうだけで済む。
// ・死んだエンティティは削除予約された状態でまだ生きている(実際に消えるのは
//   次フレームの BeginFrame)ので、ここでコンポーネントを引ける。
// ・カスタムタスクで登録している。1フレームに1回だけ走らせたいのと、
//   ActiveCustomTask は ActiveTag を読む扱いになるため、
//   死亡を積む側(ActiveTask)より必ず後ろに並ぶ。
// ・読み終わったらここでクリアする。消費するのがこのシステムだけだからで、
//   他にも死亡を見たいものが出てきたら HitEventResource と同じく
//   PreUpdate でクリアする形へ移すこと。
//==============================================================================
void DeathEffectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<DeathEffectComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<DeathEventResource>()) return;

			auto& _deathEvents = a_ctx.pWorld->GetResource<DeathEventResource>();
			if (_deathEvents.events.empty()) return;

			for (const DeathEvent& _event : _deathEvents.events)
			{
				if (_event.entity == Engine::ECS::Limits::INVALID_ENTITY) continue;

				// エフェクトを登録していない相手は静かに消えるだけ
				if (!a_ctx.pWorld->HasComponent<DeathEffectComponent>(_event.entity)) continue;

				auto* _pDeathEffect = a_ctx.pWorld->RefData<DeathEffectComponent>(_event.entity);
				if (!_pDeathEffect) continue;

				App::Utility::SpawnPrefabAt(
					*a_ctx.pWorld,
					*a_ctx.pServices->pResourceManager,
					_pDeathEffect->prefabGUID,
					_pDeathEffect->prefabHandle,
					_event.pos);
			}

			_deathEvents.Clear();
		}
	);
}
