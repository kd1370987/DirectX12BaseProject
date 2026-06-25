#include "GBufferPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"

namespace Engine::Graphics
{
	void AddGBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
			uint8_t animationIndex;
		};
		auto _spPassData = std::make_shared<RuntimeData>();

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "GBuffer";
		RGRasterPassBuilder _rpBuilder(&_node, &a_rg);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/GBufferShader/GBufferVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		// 依存関係構築
		_rpBuilder.Read("Depth", AccessType::Depth_Read, LoadOp::Load, StoreOp::Store);

		_rpBuilder.Write("GBufferAlbedo", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferNormal", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferMaterial", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferEmissiv", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		_rpBuilder.Write("GBufferVelocity", AccessType::RTV, LoadOp::Clear, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("GBufferStatic", _spPassData->staticIndex);
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		auto& _aPso = _rpBuilder.CreatePSODesc("GBufferAnimation", _spPassData->animationIndex);
		_rpBuilder.SetVS(_aPso, "Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso", D3D12::Input::AnimationInputLayout);
		_rpBuilder.SetPS(_aPso, "Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		_aPso.DepthEnable(true);
		_aPso.DepthWriteMask(false);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			a_pCtx->BindHeap();
			a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

			CameraData _cbCam = a_pGE->GetCameraData();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();
			a_pCtx->BindCB()->BindAndAttachDataRootCBV<CameraData>(_pCmd, 0, _cbCam);
			a_pCtx->BindInstanceBuffer(2);
			a_pCtx->BindSubsetBuffer(3);
			a_pCtx->BindBonePalletBuffer(4);
			
			// 描画
			a_pGE->DrawQueue(a_pCtx, a_passIndex);
		};

		// パス登録
		a_rg.AddPassNode(a_phase, _node);
	}
}