#include "ModelFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/ModelComponent.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
//#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Loader/Model/ModelLoader.h"

void ModelFixupSystem::Init(Engine::ECS::World& a_world)
{
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
				if(_modelComp.modelGUID != Engine::DefaultGUID)
				{
					//_modelComp.handle = Engine::Resource::ModelManager::Instnace().Load(_modelComp.modelGUID);
					_modelComp.handle = Engine::Resource::ModelLoader::Load(_modelComp.modelGUID);
				}
			}
		}
	);
}

