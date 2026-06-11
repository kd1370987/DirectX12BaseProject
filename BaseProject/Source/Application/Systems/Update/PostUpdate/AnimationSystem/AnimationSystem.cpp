#include "AnimationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../../Engine/Animation/AnimationMatrixManager/AnimationMatrixManager.h"

void AnimationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ModelComponent, AnimatorComponent, NodePoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"AnimationSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			AnimatorComponent* a_animatorArray,
			NodePoseComponent* a_NodePoseArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				AnimatorComponent& _aniComp = a_animatorArray[_i];
				NodePoseComponent& _nodeComp = a_NodePoseArray[_i];

				const auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;

				// アニメーション取得
				if (_pModel->GetAnimationHandles().size() <= _aniComp.clipID) continue;
				const auto& _aniHandle = _pModel->GetAnimationHandles()[_aniComp.clipID];
				const auto* _pAni = Engine::Resource::ResourceManager::Instance().Get(_aniHandle);
				if (!_pAni) continue;

				// 行列取得
				Engine::Animation::NodePose* _pNodePoseVec = 
				Engine::Animation::AnimationMatrixManager::Instance().AccessNodePoseVec(_nodeComp.nodeRange);
				if (!_pNodePoseVec) return;

				// すべてのアニメーションノードの行列保管を実行する
				for (size_t _j = 0; _j < _pAni->nodes.size(); ++_j)
				{
					UINT _idx = _pAni->nodes[_j].nodeOffset;
					Engine::Animation::Interpolate(_pAni->nodes[_idx], _aniComp.time, _pNodePoseVec[_idx].local);
				}

				// アニメーションタイム進行
				_aniComp.time += a_dt * _aniComp.speed;

				if (_aniComp.time >= _pAni->maxLength)
				{
					if (_aniComp.isLoop != 0)
					{
						_aniComp.time = 0.0f;
					}
					else
					{
						_aniComp.time = _pAni->maxLength;
					}
				}
			}
		}
	);
}