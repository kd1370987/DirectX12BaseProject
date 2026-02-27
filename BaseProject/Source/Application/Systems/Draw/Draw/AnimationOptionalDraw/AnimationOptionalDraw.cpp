#include "AnimationOptionalDraw.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Engine/Graphics/GraphicResource/Resource/Model/Model.h"
#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

void AnimationOptionalDrawSystem::Run(World& a_world, float a_dt)
{
	a_world.ForEach<WorldMatrixComponent, ModelComponent, SkeletonPoseComponent, AnimatorComponent,NodePoseComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			WorldMatrixComponent* a_matArray,
			ModelComponent* a_modelArray,
			SkeletonPoseComponent* a_skeArray,
			AnimatorComponent* a_aniArray,
			NodePoseComponent* a_nodePoseArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				WorldMatrixComponent& _matComp = a_matArray[_i];
				ModelComponent& _modelComp = a_modelArray[_i];
				SkeletonPoseComponent& _skeComp = a_skeArray[_i];
				AnimatorComponent& _aniComp = a_aniArray[_i];
				NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];

				// 描画アイテム
				DrawItem _item = {};
				_item.worldMat = _matComp.worldMat;
				_item.colorScale = _modelComp.colorScale;
				_item.emissiveScale = _modelComp.emissiveScale;

				// モデル取得
				auto* _model = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);
				if (!_model) return;

				// 全ノード
				auto& _workNodes = _nodePoseComp;
				auto& _dataNodes = _model->originalNodes;

				// ボーンノード	
				_item.pBoneMatrices = _skeComp.palette;
				_item.boneCount = 300;
				

				// 描画ノード
				for (auto& _nodeIdx : _model->drawMeshNodeIndices)
				{
					for (auto& _meshIdx : _dataNodes[_nodeIdx].meshIndices)
					{
						// 描画メッシュ取得
						_item.pMesh = _model->spMeshVec[_meshIdx].get();
						if (!_item.pMesh) continue;

						// ワールド行列
						DXSM::Matrix _nodeTransMat(_workNodes.world[_nodeIdx]);
						DXSM::Matrix _worldMat(_matComp.worldMat);
						DXSM::Matrix _mat = _nodeTransMat * _worldMat;
						_item.worldMat = _mat;

						for (UINT _subIdx = 0; _subIdx < _item.pMesh->GetSubsets().size(); ++_subIdx)
						{
							// マテリアルセット
							if (_item.pMesh->GetSubsets()[_subIdx].faceCount == 0) continue;
							_item.pMaterial = &_model->materials[_item.pMesh->GetSubsets()[_subIdx].materialNumber];
							_item.subIdx = _subIdx;

							// 描画アイテムキューに送信
							
							// アルファモードによって描画先を変える
							Alpha _mode = _model->materials[_item.pMesh->GetSubsets()[_subIdx].materialNumber].alphaMode;
							switch (_mode)
							{
							case Alpha::Opaque:
								RenderContext::Instance().AddItem(RenderQueueType::AnimationOpaque, _item);
								break;
							case Alpha::Mask:
								RenderContext::Instance().AddItem(RenderQueueType::AnimationOpaque, _item);
								break;
							case Alpha::Blend:
								RenderContext::Instance().AddItem(RenderQueueType::AnimationTransparent, _item);
								break;
							default:
								break;
							}
						}
					}
				}
			}
		}
	);
}
