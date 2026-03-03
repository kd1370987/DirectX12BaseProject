#include "SkinningSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Resource/SkeletonPoseComponent.h"

#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"


void SkinningSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<ModelComponent, NodePoseComponent,SkeletonPoseComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			ModelComponent* a_modelArray,
			NodePoseComponent* a_nodePoseArray,
			SkeletonPoseComponent* a_skePoseArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ModelComponent& _modelComp = a_modelArray[_i];
				NodePoseComponent& _nodeComp = a_nodePoseArray[_i];
				SkeletonPoseComponent& _skeComp = a_skePoseArray[_i];

				// モデル取得
				const auto* _model = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);
				if (!_model) return;

				// 全ノード
				auto& _workNodes = _nodeComp;
				const auto& _dataNodes = _model->originalNodes;

				for (auto& _s : _skeComp.palette)
				{
					_s = DXSM::Matrix::Identity;
				}

				// ボーンノード
				for (auto&& _nodeIdx : _model->boneNodeIndices)
				{
					const auto& _dataNode = _dataNodes[_nodeIdx];
					
					DXSM::Matrix _nodeWorldMat(_nodeComp.world[_nodeIdx]);
					DXSM::Matrix _invMat(_dataNodes[_nodeIdx].boneInverseWorldMatrix);
					_skeComp.palette[_dataNode.boneIndex] = _invMat * _nodeWorldMat;
				}
			}
		}
	);
}