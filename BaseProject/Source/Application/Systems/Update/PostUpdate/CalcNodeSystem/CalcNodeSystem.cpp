#include "CalcNodeSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

void CalcNodeSystem::Init(Engine::ECS::World& a_world)
{
	a_world.RegisterTask<const ModelComponent,const AnimatorComponent, NodePoseComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt, 
			const ModelComponent* a_modelArray,
			const AnimatorComponent* a_animatorArray,
			NodePoseComponent* a_nodePoseArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				const AnimatorComponent& _aniComp = a_animatorArray[_i];
				NodePoseComponent& _nodeComp = a_nodePoseArray[_i];

				// モデル取得
				auto* _pModel = Engine::Resource::ModelManager::Instnace().GetModel(_modelComp.handle);
				if (!_pModel) continue;

				// ルートから開始
				for (int _rootIdx : _pModel->GetRootNodeVec())
				{
					Engine::Animation::CalcNodeMatrix(
						_rootIdx,
						-1,
						_pModel,
						_nodeComp.local,
						_nodeComp.world
					);
				}

			}
		}
	);
}