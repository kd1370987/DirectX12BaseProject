#include "SimpleDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void SimpleDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, const ModelComponent>(
		Engine::ECS::ESystemType::Draw,
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_worldMatArray,
			const ModelComponent* a_modelArray
		)
		{
			auto* _pRCT = Engine::MainEngine::Instance().RefRenderContext();

			auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
			if (!_pGE) return;
			auto* _pRG = _pGE->RefRenderGraph();
			if (!_pRG) return;

			uint8_t _zpreIdx = _pRG->GetPassIndex("ZPre");
			uint8_t _opeqIdx = _pRG->GetPassIndex("GBuffer");
			uint8_t _fwIdx = _pRG->GetPassIndex("ForwardLighting");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];

				// 描画アイテム
				Engine::Graphics::DrawItem _item = {};
				_item.worldMat = _worldMatComp.worldMat;
				_item.colorScale = _modelComp.colorScale;
				_item.emissiveScale = _modelComp.emissiveScale;


				// モデル取得
				auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_model) continue;
				const DXSM::Matrix _worldMat(_worldMatComp.worldMat);
				// 描画コマンド取得
				const auto& _drawCmdVec = _model->GetDrawCommandVec();
				for (auto& _cmd : _drawCmdVec)
				{
					Engine::Graphics::LightWeightDrawItem _item = {};
					_item.worldMat = _worldMatComp.worldMat;
					_item.colorScale = _modelComp.colorScale;
					_item.emissiveScale = _modelComp.emissiveScale;
					
					_item.sortKey.bits.meshID = _cmd.meshRawID;
					_item.sortKey.bits.materialID = _cmd.materialRawID;
					_item.sortKey.bits.psoID = 0;		// 仮でアニメーションナシは0ありは1
					
					_item.subIndex = _cmd.subIdx;
					
					// 変換
					DXSM::Matrix _nodeTransMat(_model->GetOriginalNodeVec()[_cmd.nodeIndex].worldTransform); DirectX::XMStoreFloat4x4(&_item.worldMat, _nodeTransMat * _worldMat);

					switch (_cmd.alphaMode)
					{
					case Engine::Resource::Alpha::Opaque:
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pRCT->AddItem(_item);
						_item.sortKey.bits.passIndex = _opeqIdx;
						_pRCT->AddItem(_item);
						break;
					case Engine::Resource::Alpha::Mask:
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pRCT->AddItem(_item);
						_item.sortKey.bits.passIndex = _opeqIdx ;
						_pRCT->AddItem(_item);
						break;
					case Engine::Resource::Alpha::Blend:
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pRCT->AddItem(_item);
						_item.sortKey.bits.passIndex = _fwIdx;
						_pRCT->AddItem(_item);
						break;
					default:
						break;
					}



				}

				// ノード
				auto& _dataNodes = _model->GetOriginalNodeVec();

				// 描画ノード
				for (auto& _nodeIdx : _model->GetDrawNodeVec())
				{
					for (auto& _meshIdx : _dataNodes[_nodeIdx].meshIndices)
					{
						// 描画メッシュ取得
						const auto& _meshHandle = _model->GetMeshHandles()[_meshIdx];
						_item.pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandle);
						if (!_item.pMesh) continue;

						// ノードのワールド行列を計算
						DXSM::Matrix _nodeTransMat(_dataNodes[_nodeIdx].worldTransform);
						DXSM::Matrix _worldMat(_worldMatComp.worldMat);
						_item.worldMat = _nodeTransMat * _worldMat;

						// サブセットごとに描画
						for (UINT _subIdx = 0; _subIdx < _item.pMesh->GetMetaData().subsets.size(); ++_subIdx)
						{
							// 面が一枚もない場合はスキップ
							if (_item.pMesh->GetMetaData().subsets[_subIdx].faceCount == 0) continue;
							const auto& _mateHandle = _model->GetMaterialHandles()[_item.pMesh->GetMetaData().subsets[_subIdx].materialNumber];
							_item.pMaterial = Engine::Resource::ResourceManager::Instance().Get(_mateHandle);
							_item.subIdx = _subIdx;

							// アルファモードによって描画先を変える
							Engine::Resource::Alpha _mode = _item.pMaterial->alphaMode;
							switch (_mode)
							{
							case Engine::Resource::Alpha::Opaque:
								_pRCT->AddItem(RenderQueueType::Opaque, _item);
								break;
							case Engine::Resource::Alpha::Mask:
								_pRCT->AddItem(RenderQueueType::Opaque, _item);
								break;
							case Engine::Resource::Alpha::Blend:
								_pRCT->AddItem(RenderQueueType::Transparent, _item);
								break;
							default:
								break;
							}
						}
					}
				}

			}
		},
		Engine::ECS::Exclude<AnimatorComponent>()
	);
}
