#include "StateMachinFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

#include "../../../../../Engine/Resource/Loader/StateMachineAsset/StateMachineAssetLoader.h"
#include "../../../../../Engine/Resource/Manager/InstancePoolManager/InstancePoolManager.h"

#include "../../../../../Engine/Resource/Data/StateMachineAsset/StateMachineAsset.h"

void StateMachinFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<StateMachineComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"StateMachinFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
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
					_smComp.stateMachineHandle = Engine::Resource::StateMachineAssetLoader::Load(_smComp.stateMachineGUID);
					_smComp.instanceHandle = Engine::Resource::InstancePoolManager::Instance().Allocate<Engine::Resource::StateMachinInstance>();
				}
			}
		}
	);
}
