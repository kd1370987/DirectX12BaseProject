#include "DeathEffectSystem.h"

#include "Application/ECS/World/World.h"

#include "Application/Components/Character/DeathEffectComponent.h"
#include "Application/Components/Character/HealthComponent.h"
#include "Application/Components/Collision/ExplodeOnHitComponent.h"
#include "Application/InstanceResource/DeathEventResource.h"
#include "Application/Utility/EffectSpawnHelper.h"

//==============================================================================
// DeathEffectSystem
//
// そのフレームに死んだエンティティを見て、DeathEffectComponent に登録されている
// EffectAsset を死んだ位置へ出す。ここは出すだけで、演出の中身はアセットが持つ。
//
// ・出せるのは EffectAsset だけ。時間差で複数のものを出す演出もパーツごとの
//   EffectTiming で表せるので、演出のためだけにプレハブを1枚挟む必要はない。
//   出したエンティティは destroyOnFinish で自分から消えるので後片付けも要らない
//   (SpawnEffectAt が組み立てる)。
// ・死亡は DeathEventResource から受け取る。死因を持つシステム
//   (体力切れ = HealthSystem / 着弾 = ExplodeOnHitSystem)は
//   「誰がどこで死んだか」を積むだけでよく、エフェクトの存在を知らない。
//   死因が増えても積んでもらうだけで済む。
// ・死んだ本人はまだ生きている。体力切れの場合は HealthComponent が死亡状態に
//   なっただけで、実際に消えるのは releaseDelay 秒あと(DeathStateSystem)。
//   弾の場合は解放予約された状態で、消えるのは次フレームの BeginFrame。
//   どちらもこの時点ではコンポーネントを引ける。
// ・カスタムタスクで登録している。1フレームに1回だけ走らせたいため。
//
// ・読み込みに HealthComponent と ExplodeOnHitComponent を挙げている。
//   このシステム自体はどちらも触らないが、死亡を積むのがその2つを書くシステム
//   (HealthSystem / ExplodeOnHitSystem)なので、こう書いておくと
//   「書く側 → 読む側」の辺が張られて必ず後ろに回る。
//
//   挙げていないと、依存の無いタスクとして真っ先に実行されてしまう。
//   同じ PostUpdate 帯でも順序は依存でしか決まらないので、
//   積まれる前に読んで毎フレーム空振りし、エフェクトが一切出なくなる。
// ・クリアはしない。スコア加算(ScoreSystem)も同じ死亡を読むようになったので、
//   HitEventResource と同じく捨てる係を分けてある(DeathEventClearSystem / PreUpdate)。
//   先に読んだ方が消す形だと、読み手が増えたときに登録順で動いたり動かなかったりする。
//==============================================================================
void DeathEffectSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<DeathEffectComponent, HealthComponent, ExplodeOnHitComponent>{},
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

				const auto* _pDeathEffect = a_ctx.pWorld->RefData<DeathEffectComponent>(_event.entity);
				if (!_pDeathEffect) continue;

				if (_pDeathEffect->effectGUID == Engine::DefaultGUID) continue;

				// 再生用の最小エンティティをその場に出す(実体化は次の BeginFrame)
				if (!App::Utility::SpawnEffectAt(
						*a_ctx.pWorld,
						_pDeathEffect->effectGUID,
						_event.pos))
				{
					ENGINE_LOG("[DeathEffect] エフェクトを出せなかった : %s",
						_pDeathEffect->effectGUID.String().c_str());
				}
			}
		}
	);
}
