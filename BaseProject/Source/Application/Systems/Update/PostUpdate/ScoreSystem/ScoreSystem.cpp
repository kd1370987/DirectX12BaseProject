#include "ScoreSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/ScoreTargetComponent.h"
#include "Application/Components/Character/HealthComponent.h"
#include "Application/Components/Collision/ExplodeOnHitComponent.h"
#include "Application/InstanceResource/DeathEventResource.h"
#include "Application/Game/GameManager/GameManager.h"

//==============================================================================
// ScoreSystem
//
// そのフレームに死んだものを見て、倒した相手ぶんのスコアを足す。
//
// ・点数を持っているのは倒された側(ScoreTargetComponent)。
//   強い相手ほど高い、という差はシーンの中身の話なので、
//   足す側は「相手がいくら持っているか」を聞くだけにしてある。
//   敵の種類が増えてもここは触らなくてよい。
//
// ・ScoreTargetComponent が付いていないものは数えない。
//   自分から消える弾やエフェクト、破壊できる置物なども同じ死亡イベントを積むので、
//   印が無いものまで数えると「何を倒したのか分からない点数」が入る。
//
// ・二重加算は倒された側の isScored で止める。
//   体力切れの死亡は「死亡状態にしてから releaseDelay 秒後に解放」という作りで、
//   本人は数フレーム生き残る。その間に爆風などでもう一度死亡が積まれても、
//   点数が入るのは最初の1回だけにする。
//
// ・実行帯は PostUpdate。死亡を積むのは HealthSystem / ExplodeOnHitSystem で、
//   消すのは次フレーム PreUpdate の DeathEventClearSystem。
//
//   読み込みにその2つが書くコンポーネント(HealthComponent / ExplodeOnHitComponent)を
//   挙げているのは、順序を「書く側 → 読む側」の辺で縛るため。
//   挙げないと依存の無いタスクとして先に走ってしまい、
//   積まれる前に読んで毎フレーム空振りする。
//
// ・貯め先はワールドのリソースではなく GlobalGameContext(GameManager が持つ)。
//   リザルトへ持っていく数字なので、シーンを切り替えると作り直される
//   ワールドのリソースに置くと消えてしまう。
//==============================================================================
void ScoreSystem::Init(Engine::ECS::World& a_world)
{
	// コンポーネントを回さないのでカスタムタスクで登録する(フレームに1回だけ走る)
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PostUpdate,
		Engine::ECS::ReadList<HealthComponent, ExplodeOnHitComponent>{},
		Engine::ECS::WriteList<ScoreTargetComponent>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<DeathEventResource>()) return;

			auto& _gameData = App::Game::GameManager::Instance().RefGameData();

			const auto& _deathEvents = a_ctx.pWorld->GetResource<DeathEventResource>();
			if (_deathEvents.events.empty()) return;

			for (const DeathEvent& _event : _deathEvents.events)
			{
				if (_event.entity == Engine::ECS::Limits::INVALID_ENTITY) continue;

				// 倒す相手として置かれていないものは数えない。
				// RefData は持っていないコンポーネントでも非nullを返すので、
				// 必ず HasComponent で確かめてから引くこと
				if (!a_ctx.pWorld->HasComponent<ScoreTargetComponent>(_event.entity)) continue;

				auto* _pTarget = a_ctx.pWorld->RefData<ScoreTargetComponent>(_event.entity);
				if (!_pTarget) continue;

				// 死んでから消えるまでの間にもう一度積まれても、入るのは1回だけ
				if (_pTarget->isScored) continue;
				_pTarget->isScored = true;

				_gameData.AddScore(_pTarget->score);
			}
		}
	);
}
