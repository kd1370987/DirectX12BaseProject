#include "CalcNodeSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"

#include "Engine/GraphicResource/Resource/Model/Model.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"
#include "Engine/Animation/AnimationEvaluator/AnimationEvaluator.h"


void CalcNodeSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<ModelComponent, AnimatorComponent,NodePoseComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			ModelComponent* a_modelArray,
			AnimatorComponent* a_animatorArray,
			NodePoseComponent* a_nodePoseArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ModelComponent& _modelComp = a_modelArray[_i];
				AnimatorComponent& _aniComp = a_animatorArray[_i];
				NodePoseComponent& _nodeComp = a_nodePoseArray[_i];

				// モデル取得
				auto* _pModel = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);

				// ルートから開始
				for (int _rootIdx : _pModel->rootNodeIndices)
				{
					Animation::CalcNodeMatrix(
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