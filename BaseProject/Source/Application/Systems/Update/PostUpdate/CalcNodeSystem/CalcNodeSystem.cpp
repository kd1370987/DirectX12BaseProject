#include "CalcNodeSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"

#include "Engine/GraphicResource/Resource/Model/Model.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"


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

				auto* _pModel = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);

				std::function<void(int)> _CalcNode;
				_CalcNode = [&](int nodeIdx)
					{
						auto& _data = _pModel->originalNodes[nodeIdx];

						// 親合成
						if (_data.parent >= 0)
						{
							DXSM::Matrix _l(_nodeComp.local[nodeIdx]);
							DXSM::Matrix _p(_nodeComp.world[_data.parent]);
							_nodeComp.world[nodeIdx] = (_l * _p);
						}
						else
						{
							_nodeComp.world[nodeIdx] = _nodeComp.local[nodeIdx];
						}

						// 子供再帰
						for (int _child : _data.children)
						{
							_CalcNode(_child);
						}
					};

				// ルートから開始
				for (int root : _pModel->rootNodeIndices)
				{
					_CalcNode(root);
				}

			}
		}
	);
}