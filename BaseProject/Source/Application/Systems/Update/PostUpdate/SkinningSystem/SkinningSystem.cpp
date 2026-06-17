#include "SkinningSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Resource/SkeletonPoseComponent.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../../Engine/Animation/AnimationMatrixManager/AnimationMatrixManager.h"
void SkinningSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ModelComponent, NodePoseComponent, SkeletonPoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"SkinningSystem",
		[&a_world](
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
				auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) return;

				// 全ノード
				const auto& _dataNodes = _pModel->GetOriginalNodeVec();

				// 全スケルタルポーズを初期化
				auto& _boneMatPool = a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
				auto _boneMatVec = _boneMatPool.RefRange(_skeComp.skeletonPoseHandle);
				for (auto& _mat : _boneMatVec)
				{
					_mat.mat = DXSM::Matrix::Identity;
				}

				// 全ノードポーズを再帰計算
				auto& _nodePosePool = a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				auto _nodePoseMatVec = _nodePosePool.RefRange(_nodeComp.nodePoseHandle);

				// ボーンノード
				for (auto& _nodeIdx : _pModel->GetBoneNodeVec())
				{
					// 【デバッグ用】ノードインデックスがスパンの範囲内かチェック
					assert(_nodeIdx < _nodePoseMatVec.size() && "SkinningSystem: _nodeIdx out of range for NodePose!");

					const auto& _dataNode = _dataNodes[_nodeIdx];

					// 【デバッグ用】ボーンインデックスがスパンの範囲内かチェック
					assert(_dataNode.boneIndex < _boneMatVec.size() && "SkinningSystem: boneIndex out of range for BoneMat!");

					DXSM::Matrix _nodeWorldMat = _nodePoseMatVec[_nodeIdx].world;
					DXSM::Matrix _invMat = _dataNodes[_nodeIdx].boneInverseWorldMatrix;
					_boneMatVec[_dataNode.boneIndex].mat = _invMat * _nodeWorldMat;
				}

			}
		}
	);
}