#include "SimpleDrawSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void SimpleDrawSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEachEx<WorldMatrixComponent,ModelComponent>(
		[&a_world, a_dt]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			WorldMatrixComponent* a_matArray,
			ModelComponent* a_modelArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				WorldMatrixComponent& _worldMatComp = a_matArray[_i];
				ModelComponent& _modelComp = a_modelArray[_i];

				// 描画アイテム
				Engine::Graphics::DrawItem _item = {};
				_item.worldMat = _worldMatComp.worldMat;
				_item.colorScale = _modelComp.colorScale;
				_item.emissiveScale = _modelComp.emissiveScale;

				// モデル取得
				//Engine::Resource::Model* _model = GraphicResourceManager::Instance().NGetModel(_modelComp.modelID);
				auto* _model = Engine::Resource::ModelManager::Instnace().RefModel(_modelComp.handle);
				if (!_model) return;

				// ノード
				auto& _dataNodes = _model->originalNodes;

				// 描画ノード
				for (auto& _nodeIdx : _model->drawMeshNodeIndices)
				{
					for (auto& _meshIdx : _dataNodes[_nodeIdx].meshIndices)
					{
						// 描画メッシュ取得
						_item.pMesh = _model->spMeshVec[_meshIdx].get();
						if (!_item.pMesh) continue;

						// ノードのワールド行列を計算
						DXSM::Matrix _nodeTransMat(_dataNodes[_nodeIdx].worldTransform);
						DXSM::Matrix _worldMat(_worldMatComp.worldMat);
						_item.worldMat = _nodeTransMat * _worldMat;

						// サブセットごとに描画
						for (UINT _subIdx = 0; _subIdx < _item.pMesh->GetSubsets().size(); ++_subIdx)
						{
							// 面が一枚もない場合はスキップ
							if (_item.pMesh->GetSubsets()[_subIdx].faceCount == 0) continue;
							_item.pMaterial = &_model->materials[_item.pMesh->GetSubsets()[_subIdx].materialNumber];
							_item.subIdx = _subIdx;

							// アルファモードによって描画先を変える
							Engine::Resource::Alpha _mode = _model->materials[_item.pMesh->GetSubsets()[_subIdx].materialNumber].alphaMode;
							switch (_mode)
							{
							case Engine::Resource::Alpha::Opaque:
								Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType::Opaque, _item);
								Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType::Debug, _item);
								break;
							case Engine::Resource::Alpha::Mask:
								Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType::Opaque, _item);
								Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType::Debug, _item);
								break;
							case Engine::Resource::Alpha::Blend:
								Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType::Transparent, _item);
								Engine::Graphics::RenderContext::Instance().AddItem(RenderQueueType::Debug, _item);
								break;
							default:
								break;
							}
						}

						// 当たり判定描画
						auto& _coll = _item.pMesh->GetCollision();
						for (auto& _cell : _coll.grid.cellVec)
						{
							Engine::Graphics::RenderContext::Instance().RefShapeDraw()->AABB(_cell.box);
						}
					}
				}
				
			}
		},
		Engine::ECS::Exclude<AnimatorComponent>()
	);

}
