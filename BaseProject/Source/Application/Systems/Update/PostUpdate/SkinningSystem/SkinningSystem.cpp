#include "SkinningSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Resource/SkeletonPoseComponent.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void SkinningSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ModelComponent, NodePoseComponent, SkeletonPoseComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			NodePoseComponent* a_nodePoseArray,
			SkeletonPoseComponent* a_skePoseArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				NodePoseComponent& _nodeComp = a_nodePoseArray[_i];
				SkeletonPoseComponent& _skeComp = a_skePoseArray[_i];

				// モデル取得
				//auto* _model = Engine::Resource::ModelManager::Instnace().GetModel(_modelComp.handle);
				auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_model) return;

				// 全ノード
				auto& _workNodes = _nodeComp;
				const auto& _dataNodes = _model->GetOriginalNodeVec();

				for (auto& _s : _skeComp.palette)
				{
					_s = DXSM::Matrix::Identity;
				}

				// ボーンノード
				for (auto& _nodeIdx : _model->GetBoneNodeVec())
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