#include "AnimationModelStartSystem.h"

#include "Engine/ECS/World/World.h"

//#include "../../../../../Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../../../Engine/Animation/AnimationMatrixManager/AnimationMatrixManager.h"

#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Resource/AnimatorComponent.h"
#include "../../../../Components/Resource/NodePoseComponent.h"
#include "../../../../Components/Resource/SkeletonPoseComponent.h"

void AnimationModelStartSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<const ModelComponent,AnimatorComponent,NodePoseComponent,SkeletonPoseComponent>(
		Engine::ECS::ESystemType::Start,
		"AnimationModelStartSystem",
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			StartTag* a_startTag,
			const ModelComponent* a_pModelArray, 
			AnimatorComponent* a_animationArray,
			NodePoseComponent* a_nodeArray, 
			SkeletonPoseComponent* a_poseArray
			
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_pModelArray[_i];
				AnimatorComponent& _animationComp = a_animationArray[_i];
				NodePoseComponent& _nodeComp = a_nodeArray[_i];
				SkeletonPoseComponent& _poseComp = a_poseArray[_i];

				// モデル取得
				auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;

				// アニメーター初期化
				_animationComp.clipID = 0;
				_animationComp.time = 0.0f;
				_animationComp.speed = 30.0f;
				_animationComp.isLoop = true;

				// ノードポーズ行列領域確保
				auto& _nodePosePool = a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
	
				// モデルのアニメーションから最大ノードを持つものを取得
				UINT _totalNodeCount = static_cast<UINT>(_pModel->GetOriginalNodeVec().size());
				_nodeComp.nodePoseHandle = _nodePosePool.AllocateRange(_totalNodeCount);

				// ノードポーズ初期化
				for (auto& _mat : _nodePosePool.RefRange(_nodeComp.nodePoseHandle))
				{
					_mat.local = DXSM::Matrix::Identity;
					_mat.world = DXSM::Matrix::Identity;
				}

				// ボーン行列領域確保
				auto& _boneMatPool = a_world.GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
				size_t _boneNodeCount = _pModel->GetBoneNodeVec().size();
				_poseComp.skeletonPoseHandle = _boneMatPool.AllocateRange(static_cast<uint32_t>(_boneNodeCount));

				// スケルタルポーズ初期化
				for (auto& _mat : _boneMatPool.RefRange(_poseComp.skeletonPoseHandle))
				{
					_mat.mat = DXSM::Matrix::Identity;
				}

			}
		}
	);
}
