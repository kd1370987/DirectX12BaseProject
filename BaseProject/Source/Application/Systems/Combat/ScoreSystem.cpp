#include "ScoreSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Combat/ScoreTargetComponent.h"
#include "Application/InstanceResource/DeathEventResource.h"
#include "Application/InstanceResource/GameDataResource.h"

namespace App::System
{
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
	//   DeathEventResource を読むと宣言して(ReadsResource)、積む2つ(書くと宣言している)の
	//   後ろへ「書く側 → 読む側」の辺で縛っている。
	//   宣言が無いと依存の無いタスクとして先に走ってしまい、
	//   積まれる前に読んで毎フレーム空振りする。
	//   (以前はリソースの依存を書けなかったので、積む側が書くコンポーネント
	//    HealthComponent / ExplodeOnHitComponent を読みに挙げて代わりにしていた)
	//
	// ・貯め先はワールドのリソースではなく GlobalGameContext(GameManager が持つ)。
	//   リザルトへ持っていく数字なので、シーンを切り替えると作り直される
	//   ワールドのリソースに置くと消えてしまう。
	//   ワールドに置いてあるのは入口(GameDataResource)だけで、中身は GameManager の持ち物。
	//==============================================================================
	void ScoreSystem::Init(App::ECS::APPWorld& a_world)
	{
		// コンポーネントを回さないのでカスタムタスクで登録する(フレームに1回だけ走る)
		a_world.ActiveCustomTask(
			Engine::ECS::ESystemType::PostUpdate,
			"ScoreSystem",
			Engine::ECS::ReadList<>{},
			Engine::ECS::WriteList<Component::ScoreTargetComponent>{},
			[](const Engine::ECS::SystemContext& a_ctx)
			{
				if (!a_ctx.pWorld) return;
				if (!a_ctx.pWorld->HasResource<InstanceResource::DeathEventResource>()) return;

				auto* _pGameData = InstanceResource::GameDataResource::Find(a_ctx.pWorld);
				if (!_pGameData) return;
				auto& _gameData = *_pGameData;

				const auto& _deathEvents = a_ctx.pWorld->GetResource<InstanceResource::DeathEventResource>();
				if (_deathEvents.events.empty()) return;

				for (const InstanceResource::DeathEvent& _event : _deathEvents.events)
				{
					if (_event.entity == Engine::ECS::Limits::INVALID_ENTITY) continue;

					// 倒す相手として置かれていないものは数えない。
					// RefData は持っていないコンポーネントなら nullptr を返す
					if (!a_ctx.pWorld->HasComponent<Component::ScoreTargetComponent>(_event.entity)) continue;

					auto* _pTarget = a_ctx.pWorld->RefData<Component::ScoreTargetComponent>(_event.entity);
					if (!_pTarget) continue;

					// 死んでから消えるまでの間にもう一度積まれても、入るのは1回だけ
					if (_pTarget->isScored) continue;
					_pTarget->isScored = true;

					_gameData.AddScore(_pTarget->score);
				}
			}
		)
		// 死亡の一覧を読む(積むのは HealthSystem / ExplodeOnHitSystem)
		.ReadsResource<InstanceResource::DeathEventResource>()
		// 数えた結果はシーンをまたぐ記録へ足す(書き先はリソースが指している GlobalGameContext)
		.WritesResource<InstanceResource::GameDataResource>();
	}
}
