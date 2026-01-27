#include "SimpleDrawSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

void SimpleDrawSystem::Run(World& a_world, float a_dt)
{
	//RenderContext::Instance().BeginPass(RenderPassID::Simple);

	a_world.ForEach<WorldMatrixComponent,ModelComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			WorldMatrixComponent* a_matArray,
			ModelComponent* a_modelArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				WorldMatrixComponent& _worldMatComp = a_matArray[_i];
				ModelComponent& _modelComp = a_modelArray[_i];

				/*RenderContext::Instance().DrawModelPass(
					_modelComp.modelID,
					_worldMatComp.worldMat,
					_modelComp.colorScale,
					_modelComp.emissiveScale
				);*/
				RenderCommand _cmd = {};
				_cmd.rootSigID = 0;
				_cmd.psoID = 0;
				_cmd.worldMat = _worldMatComp.worldMat;
				_cmd.colorScale = _modelComp.colorScale;
				_cmd.emissiveScale = _modelComp.emissiveScale;
				_cmd.modelID = _modelComp.modelID;

				auto* _model = GraphicResourceManager::Instance().NGetModelResource(_cmd.modelID);
				auto& _dataNodes = _model->originalNodes;
				
				for (auto& _nodeIdx : _model->drawMeshNodeIndices)
				{
					_cmd.nodeIndex = _nodeIdx;
					_cmd.pMesh = _dataNodes[_nodeIdx].spMesh.get();

					for (UINT _subIdx = 0; _subIdx < _cmd.pMesh->GetSubsets().size(); ++_subIdx)
					{
						// 面が一枚もない場合はスキップ
						if (_cmd.pMesh->GetSubsets()[_subIdx].faceCount == 0) continue;
						_cmd.primitiveIndex = _subIdx;
						RenderContext::Instance().AddCommand(_cmd);
					}
				}
				
			}
		}
	);

	//RenderContext::Instance().EndPass();
}
