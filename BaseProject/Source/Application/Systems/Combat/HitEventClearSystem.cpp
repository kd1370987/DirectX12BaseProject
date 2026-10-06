#include "HitEventClearSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/InstanceResource/HitEventResource.h"

namespace App::System
{
	void HitEventClearSystem::Init(App::ECS::APPWorld& a_world)
	{
		// コンポーネントを回さないのでカスタムタスクで登録する(フレームに1回だけ走る)
		a_world.RegisterCustomTask(
			Engine::ECS::ESystemType::PreUpdate,
			"HitEventClearSystem",
			Engine::ECS::ReadList<>{},
			Engine::ECS::WriteList<>{},
			[](const Engine::ECS::SystemContext& a_ctx)
			{
				if (!a_ctx.pWorld) return;
				if (!a_ctx.pWorld->HasResource<InstanceResource::HitEventResource>()) return;

				a_ctx.pWorld->RefResource<InstanceResource::HitEventResource>().Clear();
			}
		)
		.WritesResource<InstanceResource::HitEventResource>();
	}
}
