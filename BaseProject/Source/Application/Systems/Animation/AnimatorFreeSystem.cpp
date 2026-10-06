#include "AnimatorFreeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/UpperAnimatorComponent.h"

#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

namespace App::System
{
	namespace
	{
		// レイヤー1枚ぶんのパラメータの実体をプールへ返す
		void FreeLayer(Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>& a_instancePool, Component::AnimatorLayer& a_layer)
		{
			if (!a_layer.instanceHandle.IsValid()) return;

			a_instancePool.Remove(a_layer.instanceHandle);
			a_layer.instanceHandle = {};
		}
	}

	void AnimatorFreeSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ReleaseTask<Component::AnimatorComponent>(
			Engine::ECS::ESystemType::Release,
			"AnimatorFreeSystem",
			[](
				Engine::ECS::Chunk*,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ReleaseTag*,
				Component::AnimatorComponent* a_animatorArray
			)
			{
				auto& _instancePool = a_ctx.pWorld->RefResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					FreeLayer(_instancePool, a_animatorArray[_i].baseLayer);
				}
			}
		)
		// パラメータの実体のプールへ返す
		.WritesResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

		// 上に重ねるレイヤー
		a_world.ReleaseTask<Component::UpperAnimatorComponent>(
			Engine::ECS::ESystemType::Release,
			"AnimatorFreeSystem_Upper",
			[](
				Engine::ECS::Chunk*,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ReleaseTag*,
				Component::UpperAnimatorComponent* a_animatorArray
			)
			{
				auto& _instancePool = a_ctx.pWorld->RefResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					FreeLayer(_instancePool, a_animatorArray[_i].layer);
				}
			}
		)
		// パラメータの実体のプールへ返す
		.WritesResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
	}
}
