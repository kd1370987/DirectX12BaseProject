#include "ActionStateFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "Application/Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

void ActionStateFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<ActionStateComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"ActionStateFixupSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			PostDeserializeTag* a_tag,
			ActionStateComponent* a_array
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ActionStateComponent& _comp = a_array[_i];

				if (_comp.actionGUID != Engine::DefaultGUID)
				{
					// アセットロード
					_comp.actionHandle =
						Engine::Resource::ResourceManager::Instance().Load<Engine::Resource::ActionStateMachineAsset>(_comp.actionGUID);

					// 実行時インスタンス確保
					auto& _pool =
						a_world.GetResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();
					Engine::Resource::ActionStateInstance _instance = {};
					_comp.instanceHandle = _pool.Add(std::move(_instance));
				}
			}
		}
	);
}
