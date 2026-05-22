#include "AnimationOptionalDraw.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"
#include "../../../../../Engine/Graphics/RenderPass/BaseRenderPass.h"

#include "Application/Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../../Engine/Animation/AnimationMatrixManager/AnimationMatrixManager.h"
//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

void AnimationOptionalDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, const ModelComponent, const SkeletonPoseComponent, const AnimatorComponent, const NodePoseComponent>(
		Engine::ECS::ESystemType::Draw,
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_matArray,
			const ModelComponent* a_modelArray,
			const SkeletonPoseComponent* a_skeArray,
			const AnimatorComponent* a_aniArray,
			const NodePoseComponent* a_nodePoseArray
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

			const uint8_t _zpreAni = _zprePass->GetPSOIndex("ZPreAnimation");
			const uint8_t _gbuffAni = _gbufferPass->GetPSOIndex("GBufferAnimation");
			//const uint8_t _fwStatic = _fwPass->GetPSOIndex("ForwardLithingPSO");

			Engine::Graphics::LightWeightDrawItem _item = {};
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _matComp = a_matArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];
				const SkeletonPoseComponent& _skeComp = a_skeArray[_i];
				const AnimatorComponent& _aniComp = a_aniArray[_i];
				const NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];

				// モデル取得
				auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);;
				if (!_model) continue;

				// ボーン ノード
				auto* _nodeMatVec = Engine::Animation::AnimationMatrixManager::Instance().AccessNodePoseVec(_nodePoseComp.nodeRange);

				// 行列取得
				const DXSM::Matrix _worldMat(_matComp.worldMat);
				// 描画コマンド取得
				const auto& _drawCmdVec = _model->GetDrawCommandVec();
				for (auto& _cmd : _drawCmdVec)
				{
					_item.worldMat = _matComp.worldMat;
					_item.colorScale = _modelComp.colorScale;
					_item.emissiveScale = _modelComp.emissiveScale;

					_item.sortKey.bits.meshID = _cmd.meshRawID;
					_item.sortKey.bits.materialID = _cmd.materialRawID;

					_item.isAnimation = true;
					_item.boneRange = _skeComp.boneRange;

					_item.subIndex = _cmd.subIdx;

					// 変換
					DXSM::Matrix _nodeTransMat(_nodeMatVec[_cmd.nodeIndex].world);
					DirectX::XMStoreFloat4x4(&_item.worldMat, _nodeTransMat * _worldMat);

					switch (_cmd.alphaMode)
					{
					case Engine::Resource::Alpha::Opaque:
						_item.sortKey.bits.psoID = _zpreAni;
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pRCT->AddItem(_item);
						_item.sortKey.bits.psoID = _gbuffAni;
						_item.sortKey.bits.passIndex = _opeqIdx;
						_pRCT->AddItem(_item);
						break;
					case Engine::Resource::Alpha::Mask:
						_item.sortKey.bits.psoID = _zpreAni;
						_item.sortKey.bits.passIndex = _zpreIdx;
						_pRCT->AddItem(_item);
						_item.sortKey.bits.psoID = _gbuffAni;
						_item.sortKey.bits.passIndex = _opeqIdx;
						_pRCT->AddItem(_item);
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
		}); 
}