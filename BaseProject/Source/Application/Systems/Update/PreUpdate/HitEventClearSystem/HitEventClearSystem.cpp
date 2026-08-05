#include "HitEventClearSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/InstanceResource/HitEventResource.h"

void HitEventClearSystem::Init(Engine::ECS::World& a_world)
{
	// コンポーネントを回さないのでカスタムタスクで登録する(フレームに1回だけ走る)
	a_world.RegisterCustomTask(
		Engine::ECS::ESystemType::PreUpdate,
		Engine::ECS::ReadList<>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<HitEventResource>()) return;

			a_ctx.pWorld->GetResource<HitEventResource>().Clear();
		}
	);
}
