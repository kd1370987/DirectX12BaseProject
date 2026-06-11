#include "AnimationStateSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Resource/AnimatorComponent.h"

#include "../../../../Components/Resource/StateMachineComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

void AnimationStateSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ModelComponent,const StateMachineComponent,const VelocityComponent,AnimatorComponent>(
		Engine::ECS::ESystemType::Animation,
		"AnimationStateSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			const StateMachineComponent* a_stateMachineArray,
			const VelocityComponent* a_velArray,
			AnimatorComponent* a_animatorArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// 参照
				const ModelComponent& _modelComp = a_modelArray[_i];
				const StateMachineComponent& _stateComp = a_stateMachineArray[_i];
				const VelocityComponent& _velComp = a_velArray[_i];
				AnimatorComponent& _animComp = a_animatorArray[_i];


				// 状態によってアニメーションを決める
				if (_stateComp.isGround)
				{
					// 地面についている場合
				}
				else
				{
					// 地面から浮いている場合
				}

			}
		}
	);
}
