#include "StaticObjectDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"
#include "../../../../../Engine/Graphics/RenderPass/BaseRenderPass.h"

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
			//uint8_t _fwIdx = _pRG->GetPassIndex("ForwardLighting");

			const auto* _zprePass = _pRG->GetPass("ZPre");
			const auto* _gbufferPass = _pRG->GetPass("GBuffer");
			//const auto* _fwPass = _pRG->GetPass("ForwardLighting");

			const uint8_t _zpreStatic = _zprePass->GetPSOIndex("ZPreStatic");
			const uint8_t _gbuffStatic = _gbufferPass->GetPSOIndex("GBufferStatic");
			//const uint8_t _fwStatic = _fwPass->GetPSOIndex("ForwardLithingPSO");
			Engine::Graphics::LightWeightDrawItem _item = {};
			Engine::Graphics::InstanceData _instanceData = {};
			Engine::Graphics::SubSetData _subSetData = {};
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
						

					_item.sortKey.bits.meshID = _cmd.meshRawID;
					_item.sortKey.bits.materialID = _cmd.materialRawID;
					_item.isAnimation = true;
					_item.subIndex = _cmd.subIdx;
					_item.instnaceIndex = _pGE->SetInstanceData(_instanceData);
					_item.subsetIndex = _pGE->SetSubSetData(_subSetData);

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
