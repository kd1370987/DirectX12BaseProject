#include "StaticObjectDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void StaticObjectDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, const ModelComponent>(
		Engine::ECS::ESystemType::Draw,
		"StaticObjectDrawSystem",
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

			const auto* _zprePass = _pRG->GetPass("ZPre");
			const auto* _gbufferPass = _pRG->GetPass("GBuffer");

			const uint8_t _zpreStatic = _zprePass->GetPSOIndex("ZPreStatic");
			const uint8_t _gbuffStatic = _gbufferPass->GetPSOIndex("GBufferStatic");
			Engine::Graphics::LightWeightDrawItem _item = {};
			Engine::Graphics::InstanceData _instanceData = {};
			Engine::Graphics::SubSetData _subSetData = {};
			Engine::Graphics::MeshInstanceData _meshInstance = {};
			Engine::Graphics::MeshMaterial _meshMaterial = {};
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];

				// モデル取得
				auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_model) continue;

				// ワールド座標
				const DXSM::Matrix _worldMat(_worldMatComp.worldMat);

				// 描画コマンド取得
				const auto& _drawCmdVec = _model->GetDrawCommandVec();
				for (auto& _cmd : _drawCmdVec)
				{
					auto* _pMaterial = 
						Engine::Resource::ResourceManager::Instance().Accece<Engine::Resource::Material>(_cmd.materialRawID);

					DXSM::Matrix _nodeTransMat(_model->GetOriginalNodeVec()[_cmd.nodeIndex].worldTransform);
					DXSM::Matrix _mat = _nodeTransMat * _worldMat;

					_instanceData.worldMat = _mat.Transpose();
					_instanceData.prevWorldMat = _mat.Transpose();
					_instanceData.boneStartIndex = 0;
					_instanceData.boneCount = 0;
					_subSetData.baseColorScale = _modelComp.colorScale;
					_subSetData.emissiveColorScale = _modelComp.emissiveScale;
					_subSetData.metallic = _pMaterial->metallic;
					_subSetData.roughness = _pMaterial->roughness;
						
					// メッシュマテリアル
					if(_cmd.pMesh->HasMeshShaderData())
					{
						_meshMaterial.baseColor = _modelComp.colorScale; // マテリアルの色も入れろ
						_meshMaterial.emissive = _modelComp.emissiveScale;
						_meshMaterial.metallic = 0.1f;
						_meshMaterial.roughness = 0.5f;
						_meshMaterial.albedIndex = _cmd.pMaterial->baseColorTex.GetIndex();
						_meshMaterial.metaRoughnessIndex = _cmd.pMaterial->metaRoughTex.GetIndex();
						_meshMaterial.emissiveIndex = _cmd.pMaterial->emissiveTex.GetIndex();
						_meshMaterial.normalIndex = _cmd.pMaterial->normalTex.GetIndex();

						// メッシュデータ作成
						_meshInstance.worldMat = _mat.Transpose();
						_meshInstance.prevWorldMat = _mat.Transpose();
						_meshInstance.boneStartIndex = 0;
						_meshInstance.boneCount = 0;
						_meshInstance.materialOffset = _pGE->SetMeshMaterialData(_meshMaterial);
						_meshInstance.meshletOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.meshletHandle.startIndex;
						_meshInstance.vertexOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.vertexHandle.startIndex;
						_meshInstance.uviOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.uniqueVertexHandle.startIndex;
						_meshInstance.primitiveOffset = _cmd.pMesh->GetMeshShaderData().meshHandle.primitiveHandle.startIndex;
						_item.meshHandle = _cmd.pMesh->GetMeshShaderData().meshHandle;
					}

					_item.sortKey.bits.meshID = _cmd.meshRawID;
					_item.sortKey.bits.materialID = _cmd.materialRawID;
					_item.isAnimation = true;
					_item.subIndex = _cmd.subIdx;
					_item.instnaceIndex = _pGE->SetInstanceData(_instanceData);
					_item.subsetIndex = _pGE->SetSubSetData(_subSetData);

					_item.meshInstanceIndex = _pGE->SetInstanceData(_meshInstance);
					

					switch (_cmd.alphaMode)
					{
					case Engine::Resource::Alpha::Opaque:
						_item.sortKey.bits.psoID = _zpreStatic;
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pGE->AddItem(_item);
						_item.sortKey.bits.psoID = _gbuffStatic;
						_item.sortKey.bits.passIndex = _opeqIdx;
						_pGE->AddItem(_item);
						break;
					case Engine::Resource::Alpha::Mask:
						_item.sortKey.bits.psoID = _zpreStatic;
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pGE->AddItem(_item);
						_item.sortKey.bits.psoID = _gbuffStatic;
						_item.sortKey.bits.passIndex = _opeqIdx;
						_pGE->AddItem(_item);
						break;
					case Engine::Resource::Alpha::Blend:
						//_item.sortKey.bits.psoID = _zpreStatic;
						//_item.sortKey.bits.passIndex = _zpreIdx;
						//_pRCT->AddItem(_item);
						//_item.sortKey.bits.psoID = _fwStatic;
						//_item.sortKey.bits.passIndex = _fwIdx;
						//_pRCT->AddItem(_item);
						break;
					default:
						break;
					}
				}
			}
		},
		Engine::ECS::Exclude<AnimatorComponent, PreviousWorldMatrixComponent>()
	);
}
