#include "AnimationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"

#include "Engine/GraphicResource/Resource/Model/Model.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"


void AnimationSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<ModelComponent, AnimatorComponent, NodePoseComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			ModelComponent* a_modelArray,
			AnimatorComponent* a_animatorArray,
			NodePoseComponent* a_NodePoseArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ModelComponent& _modelComp = a_modelArray[_i];
				AnimatorComponent& _aniComp = a_animatorArray[_i];
				NodePoseComponent& _nodeComp = a_NodePoseArray[_i];

				auto* _pModel = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);

				// アニメーション取得
				std::shared_ptr<AnimationData> _spAni = nullptr;
				for (auto&& _anime : _pModel->spAnimations)
				{
					if (_anime->name == "Walk")
					{
						_spAni = _anime;;
					}
				}

				if (!_spAni) return;

				// すべてのアニメーションノードの行列保管を実行する
				for (auto& _rAnimNode : _spAni->nodes)
				{
					UINT _idx = _rAnimNode.nodeOffset;

					Animation::Interpolate(_spAni->nodes[_idx], _nodeComp.local[_idx]);
				}

				// アニメーションタイム進行
				_aniComp.time += _aniComp.blendTime;

				if (_aniComp.time >= _spAni->maxLength)
				{
					if (_aniComp.isLoop != 0)
					{
						_aniComp.time = 0.0f;
					}
					else
					{
						_aniComp.time = _spAni->maxLength;
					}
				}
			}
		}
	);
}