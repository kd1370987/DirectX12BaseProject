#include "ProjectileLifeTimeSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/Weapon/Projectile/ProjectileComponent.h"

//==============================================================================
// ProjectileLifeTimeSystem
//
// 弾(ProjectileComponent)の生存時間を dt(秒)ぶん減らし、尽きたら自分を消す。
// エフェクト等は出さず、削除するだけ。
//
// ・lifeTime はそのまま残り時間として減らしていく。プレハブに書いてある値は
//   実体化のたびにコピーされるので、カウントダウンしても設計値は壊れない。
//   インスペクターに出るのは「残り何秒か」になる。
// ・lifeTime が負の値なら無期限として扱い、何もしない。
//   0 まで減ったフレームで必ず消えるので、生きているエンティティの lifeTime が
//   負のまま残ることはなく、初期値の負値と区別がつく(デバッグ用の逃げ道)。
// ・削除は AddRemoveEntity で予約する。チャンクを反復している最中に消すと
//   配列が詰め替えられて壊れるため(ExplodeOnHitSystem の自壊と同じ流れ)。
// ・当たり判定で消える ExplodeOnHitSystem と同じ PostUpdate 帯に置く。
//==============================================================================
void ProjectileLifeTimeSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<ProjectileComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"ProjectileLifeTimeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			ProjectileComponent* a_projectileArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ProjectileComponent& _projectile = a_projectileArray[_i];

				// 負の値は無期限
				if (_projectile.lifeTime < 0.0f) continue;

				// 残り時間を減らす
				_projectile.lifeTime -= a_ctx.dt;

				// 尽きたら削除を予約する
				if (_projectile.lifeTime <= 0.0f)
				{
					a_ctx.pWorld->AddRemoveEntity(a_pChunk->entityData[_i]);
				}
			}
		}
	);
}
