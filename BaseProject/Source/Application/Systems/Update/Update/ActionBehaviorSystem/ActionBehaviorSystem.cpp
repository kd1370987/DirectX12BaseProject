#include "ActionBehaviorSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ActionStateComponent.h"
#include "Application/Components/Force/VelocityComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

void ActionBehaviorSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActionStateComponent, VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		"ActionBehaviorSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ActionStateComponent* a_stateArray,
			VelocityComponent* a_velArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ActionStateComponent& _state = a_stateArray[_i];
				VelocityComponent& _vel = a_velArray[_i];

				const auto* _pSM =
					Engine::Resource::ResourceManager::Instance().Get(_state.actionHandle);
				if (!_pSM) continue;

				const auto* _node = _pSM->GetStateNode(_state.currentStateHash);
				if (!_node) continue;

				// このステート中に移動できないなら水平速度を止める。
				// 移動できるなら状態ごとの速度倍率を掛ける(=状態が行動を決める)。
				if (!_node->canMove)
				{
					_vel.value.x = 0.0f;
					_vel.value.z = 0.0f;
				}
				else
				{
					_vel.value.x *= _node->moveSpeedScale;
					_vel.value.z *= _node->moveSpeedScale;
				}
			}
		}
	);
}
