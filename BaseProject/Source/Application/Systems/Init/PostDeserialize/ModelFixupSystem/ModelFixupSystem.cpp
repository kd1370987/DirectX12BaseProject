#include "ModelFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/ModelComponent.h"

#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

void ModelFixupSystem::Init(Engine::ECS::World& a_world)
{
	//a_world.RegisterTask<const PostDeserializeTag, ModelComponent>(
	a_world.PostDeserializeTask<ModelComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			PostDeserializeTag* a_tag,
			ModelComponent* a_modelArray
		)
		{
			for(size_t _i = 0; _i < a_count; ++_i)
			{
				ModelComponent& _modelComp = a_modelArray[_i];

				// モデルをGUIDから取得してロードした結果のハンドルを取得
				_modelComp.handle = Engine::Resource::ModelManager::Instnace().Load(_modelComp.modelGUID);
			}
		}
	);
}

void ModelFixupSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<PostDeserializeTag, ModelComponent>(
		[&a_world, a_dt](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			PostDeserializeTag* a_tag,
			ModelComponent* a_modelArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ModelComponent& _modelComp = a_modelArray[_i];

				// モデルをGUIDから取得してロードした結果のハンドルを取得
				_modelComp.handle = Engine::Resource::ModelManager::Instnace().Load(_modelComp.modelGUID);
			}
		}
	);
}
