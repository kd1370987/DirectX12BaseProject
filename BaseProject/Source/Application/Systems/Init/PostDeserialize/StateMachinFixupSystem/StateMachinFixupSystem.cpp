#include "StateMachinFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

#include "../../../../../Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

void StateMachinFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<StateMachineComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"StateMachinFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			StateMachineComponent* a_stateMachinArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				StateMachineComponent& _smComp = a_stateMachinArray[_i];

				// モデルをGUIDから取得してロードした結果のハンドルを取得
				if (_smComp.stateMachineGUID != Engine::DefaultGUID)
				{
					// ステートマシンロード
					_smComp.stateMachineHandle = a_ctx.pServices->pResourceManager->Load<Engine::Resource::AnimatorAsset>(_smComp.stateMachineGUID);

					// インスタンス確保
					 auto& _stateInstancePool = 
						 a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachinInstance>>();
					 Engine::Resource::StateMachinInstance _instance = {};
					 _smComp.instanceHandle = _stateInstancePool.Add(std::move(_instance));
				}
			}
		}
	);
}
