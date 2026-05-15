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
		[](
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
				//auto* _pModel = Engine::Resource::ModelManager::Instnace().GetModel(_modelComp.handle);
				auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;

				// アニメーター初期化
				_animationComp.clipID = 0;
				_animationComp.time = 0.0f;
				_animationComp.speed = 30.0f;
				_animationComp.isLoop = true;

				// ノード作成
				_nodeComp.nodeRange = Engine::Animation::AnimationMatrixManager::Instance().AllocateNodePoseVec(_modelComp.handle);
				auto* _pNodeVec = Engine::Animation::AnimationMatrixManager::Instance().AccessNodePoseVec(_nodeComp.nodeRange);
				for (int _i = 0; _i < _nodeComp.nodeRange.rangeSize; ++_i)
				{
					_pNodeVec[_i].local = DXSM::Matrix::Identity;
					_pNodeVec[_i].world = DXSM::Matrix::Identity;
				}

				// スケルタルポーズ初期化
				_poseComp.boneRange = Engine::Animation::AnimationMatrixManager::Instance().AllocateBoneMatVec(_modelComp.handle);
				auto* _pBoneVec = Engine::Animation::AnimationMatrixManager::Instance().AccessBoneMatVec(_poseComp.boneRange);
				for (int _i = 0; _i < _poseComp.boneRange.rangeSize; ++_i)
				{
					_pBoneVec[_i] = DXSM::Matrix::Identity;
				}

			}
		}
	);
}
