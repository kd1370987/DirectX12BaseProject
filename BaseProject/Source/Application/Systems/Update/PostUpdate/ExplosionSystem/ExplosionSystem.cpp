#include "ExplosionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Effect/ExplosionComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Utility/PrefabSpawnHelper.h"

//==============================================================================
// ExplosionSystem
//
// ExplosionComponent を持つエンティティの経過時間を進め、
// パーツごとの emitTime に達したらそのプレハブを自分と同じ場所に出す。
// 全パーツを出し終えたら自分を消す(進行役なので出し切ったら用済み)。
//
// ・出すのは遅延生成コマンド(SpawnPrefabAt)。反復中に実体化するとアーキタイプが
//   壊れるため、実際に出てくるのは次フレームの BeginFrame。
//   出したものは別エンティティなので、こちらが消えても道連れにはならない。
// ・プレハブ未設定のスロットは出すものが無いので、その場で発生済みとして畳む。
//   1つも設定されていなければ、生まれた次のフレームにそのまま消える。
// ・emitTime は「生成からの経過秒」であって前のパーツからの間隔ではない。
//   並び順は見ないので、時間が前後していても書いてある時刻どおりに出る。
// ・位置に WorldMatrix ではなく LocalTransform を使うのは HealthSystem と同じ理由。
//   PostUpdate 帯で WorldMatrix を読む ActiveTask を作るとシステムのソートが循環する
//   (CommitHierarchyWorldMatrixSystem が ActiveTag を読んで WorldMatrix を書いている)。
//   爆発は親を持たない単体エンティティとして出すので、ローカル座標がそのままワールド座標になる。
// ・PostUpdate 帯。出す側(DeathEffectSystem)や寿命(LifeTimeSystem)と同じ帯に置いてある。
//==============================================================================
void ExplosionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<ExplosionComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"ExplosionSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			ExplosionComponent*               a_explosionArray,
			const LocalTransformComponent*    a_trsArray
			)
		{
			if (!a_ctx.pServices || !a_ctx.pServices->pResourceManager) return;
			auto& _resourceManager = *a_ctx.pServices->pResourceManager;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ExplosionComponent&            _explosion = a_explosionArray[_i];
				const LocalTransformComponent& _trs       = a_trsArray[_i];

				_explosion.elapsedTime += a_ctx.dt;

				bool _isAllEmitted = true;

				for (PartsEffect& _parts : _explosion.parts)
				{
					if (_parts.isEmitted) continue;

					// 出すものが無いスロットは待たない
					if (_parts.prefabGUID == Engine::DefaultGUID)
					{
						_parts.isEmitted = true;
						continue;
					}

					// まだ時間が来ていない。このパーツを待つので自分はまだ消せない
					if (_explosion.elapsedTime < _parts.emitTime)
					{
						_isAllEmitted = false;
						continue;
					}

					App::Utility::SpawnPrefabAt(
						*a_ctx.pWorld,
						_resourceManager,
						_parts.prefabGUID,
						_parts.prefabHandle,
						_trs.pos);

					_parts.isEmitted = true;
				}

				// 全部炊き終わったので退場する(解放を予約。反復中に消すとチャンクが壊れるため)
				if (_isAllEmitted)
				{
					a_ctx.pWorld->AddReleaseEntity(a_pChunk->entityData[_i]);
				}
			}
		}
	);
}
