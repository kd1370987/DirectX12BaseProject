#include "DeathEffectSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/DeathEffectComponent.h"
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

			_deathEvents.Clear();
		}
	);
}
