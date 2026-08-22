#include "DeathEventClearSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/InstanceResource/DeathEventResource.h"

//==============================================================================
// DeathEventClearSystem
//
// 前のフレームに積まれた死亡イベントを捨てる。
//
// 以前は読み手が DeathEffectSystem 1つだけだったので、あちらが読み終わりに
// 自分でクリアしていた。スコア加算(ScoreSystem)も同じものを読むようになったため、
// HitEventResource と同じく「積む側・読む側・消す側」を分けている。
// 先に読んだ方がクリアしてしまうと、後から登録されたシステムが
// 登録順に振り回されて動いたり動かなかったりするため。
//==============================================================================
void DeathEventClearSystem::Init(Engine::ECS::World& a_world)
{
	// コンポーネントを回さないのでカスタムタスクで登録する(フレームに1回だけ走る)
	a_world.RegisterCustomTask(
		Engine::ECS::ESystemType::PreUpdate,
		Engine::ECS::ReadList<>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<DeathEventResource>()) return;

			a_ctx.pWorld->GetResource<DeathEventResource>().Clear();
		}
	);
}
