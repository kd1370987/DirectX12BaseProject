#include "SimpleDrawSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

void SimpleDrawSystem::Run(World& a_world, float a_dt)
{
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

				DrawItem _item = {};

				_item.worldMat = _worldMatComp.worldMat;
				_item.colorScale = _modelComp.colorScale;
				_item.emissiveScale = _modelComp.emissiveScale;

				RenderCommand _cmd = {};
				_cmd.rootSigID = 1;
				_cmd.psoID = 1;
				_cmd.worldMat = _worldMatComp.worldMat;
				_cmd.colorScale = _modelComp.colorScale;
				_cmd.emissiveScale = _modelComp.emissiveScale;

				auto* _model = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);
				auto& _dataNodes = _model->originalNodes;
				
				for (auto& _nodeIdx : _model->drawMeshNodeIndices)
				{
					_item.pMesh = _dataNodes[_nodeIdx].spMesh.get();

					_cmd.pMesh = _dataNodes[_nodeIdx].spMesh.get();

					// ノードのワールド行列を計算
					DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_nodeIdx].worldTransform);
					DirectX::XMMATRIX _wM = DirectX::XMLoadFloat4x4(&_worldMatComp.worldMat);
					DirectX::XMMATRIX _worldMat = _nodeTransMat * _wM;
					DirectX::XMStoreFloat4x4(&_cmd.worldMat,_worldMat);
					DirectX::XMStoreFloat4x4(&_item.worldMat,_worldMat);
					for (UINT _subIdx = 0; _subIdx < _cmd.pMesh->GetSubsets().size(); ++_subIdx)
					{
						// 面が一枚もない場合はスキップ
						if (_cmd.pMesh->GetSubsets()[_subIdx].faceCount == 0) continue;
						_cmd.subIdx = _subIdx;
						_cmd.pMaterial = &_model->materials[_cmd.pMesh->GetSubsets()[_subIdx].materialNumber];
						RenderContext::Instance().AddCommand(_cmd);


						_item.pMaterial = &_model->materials[_cmd.pMesh->GetSubsets()[_subIdx].materialNumber];
						RenderContext::Instance().AddItem(RenderQueueType::Opaque,_item);
					}
				}
				
			}
		}
	);

}
