#include "LifeTimeSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Common/LifeTimeComponent.h"

//==============================================================================
// LifeTimeSystem
//
// LifeTimeComponent の残り時間を dt(秒)ぶん減らし、尽きたら自分を消す。
// 弾の寿命もエフェクトの寿命もこれ1本で見る
// (以前は ProjectileLifeTimeSystem / EffectLifeTimeSystem に分かれていたが、
//  中身が同じなので寿命はコンポーネントごと共通化した)。
//
// ・value はそのまま残り時間として減らしていく。プレハブに書いてある値は
//   実体化のたびにコピーされるので、カウントダウンしても設計値は壊れない。
// ・負の値は無期限として扱い、何もしない。
// ・削除は AddReleaseEntity で予約する。チャンクを反復している最中に消すと
//   配列が詰め替えられて壊れるため。ReleaseTag を付ける形にしているのは、
//   サウンドのボイスなど借りているものを Release フェーズで返してから消すため。
// ・寿命切れは DeathEventResource へ積まない(意図的)。
//   死亡エフェクト(DeathEffectComponent)は「積まれた死亡」を見て出るので、
//   ここで積まないかぎり寿命で消えるものからは出ない。
//   弾が誰にも当たらず飛び切って消えるときは、何も無い空中で爆発が出ることになり
//   おかしいため。当たって消える場合は ExplodeOnHitSystem が当たった位置で積む。
//   時間で消える演出が要るものが出てきても、ここへ積む形にはしないこと
//   (その1行で全部の弾が空中で爆発するようになる)。
// ・生成側(ExplodeOnHitSystem / HealthSystem など)と同じ PostUpdate 帯に置く。
//   生成は遅延コマンドで次フレームの BeginFrame に実体化されるので、
//   出たフレームにいきなり消えることはない。
//==============================================================================
void LifeTimeSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<LifeTimeComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"LifeTimeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			LifeTimeComponent* a_lifeTimeArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				LifeTimeComponent& _lifeTime = a_lifeTimeArray[_i];

				// 負の値は無期限
				if (_lifeTime.value < 0.0f) continue;

				// 残り時間を減らす
				_lifeTime.value -= a_ctx.dt;

				// 尽きたら解放を予約する
				if (_lifeTime.value <= 0.0f)
				{
					a_ctx.pWorld->AddReleaseEntity(a_pChunk->entityData[_i]);
				}
			}
		}
	);
}
