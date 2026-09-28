#include "AnimatorFreeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Animation/AnimatorComponent.h"

#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

void AnimatorFreeSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ReleaseTask<AnimatorComponent>(
		Engine::ECS::ESystemType::Release,
		"AnimatorFreeSystem",
		[](
			Engine::ECS::Chunk*,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag*,
			AnimatorComponent* a_animatorArray
		)
		{
			auto& _instancePool = a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				AnimatorLayer& _layer = a_animatorArray[_i].baseLayer;
				if (!_layer.instanceHandle.IsValid()) continue;

				_instancePool.Remove(_layer.instanceHandle);
				_layer.instanceHandle = {};
			}
		}
	)
	// パラメータの実体のプールへ返す
	.WritesResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
}
