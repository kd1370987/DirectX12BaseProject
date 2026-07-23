#include "AnimationStateSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Resource/AnimatorComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

void AnimationStateSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const StateMachineComponent,AnimatorComponent>(
		Engine::ECS::ESystemType::Animation,
		"AnimationStateSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const StateMachineComponent* a_stateMachineArray,
			AnimatorComponent* a_animatorArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// 参照
				const StateMachineComponent& _stateComp = a_stateMachineArray[_i];
				AnimatorComponent& _animComp = a_animatorArray[_i];

				// ステートマシン取得
				auto* _pStateMachin = Engine::Resource::ResourceManager::Instance().Get(_stateComp.stateMachineHandle);
				if (!_pStateMachin) continue;
				const auto* _node = _pStateMachin->GetStateNode(_stateComp.currentStateHash);
				if (!_node) continue;
				
				// アニメーション取得
				_animComp.animHandle = _node->playAnimData;
				_animComp.speed = _node->speed;
				_animComp.isLoop = _node->isLoop;

				// 加算ポーズの効きもステートから流し込む
				_animComp.additiveWeight = _node->additiveWeight;
			}
		}
	);
}
