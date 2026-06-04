#include "FullScreenPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

namespace Engine::Graphics
{
	void AddFullScreenPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		struct RuntimeData
		{
			uint8_t staticIndex;
			D3D12::PipelineStateManager* pPSOManager;
			RenderGraph* pRG;
			ID3D12RootSignature* pRootSig;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		_spPassData->pRG = &a_rg;

		RenderPassNode _node = {};
		_node.name = "FullScreenPass";
		RGRasterPassBuilder _rpBuilder(&_node, &a_rg);

		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso");

		_rpBuilder.Read("QuadTexture", AccessType::SRV, LoadOp::Load, StoreOp::DontCare);
		_rpBuilder.Read("UITexture", AccessType::SRV, LoadOp::Load, StoreOp::DontCare);

		auto& _sPso = _rpBuilder.CreatePSODesc("FullScreenPass", _spPassData->staticIndex);
		
		// SetVS には InputLayout の指定が必要なため StaticLayout を渡す
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/QuadRenderingShader/QuadRenderingPS.cso");
		
		_sPso.DepthEnable(false);
		_sPso.StencilEnable(false);

		// バックバッファは登録されていないため手動で
		_sPso.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			Editor::MainEditor::Instance().StartWatch("FullScreenPass");
			a_pCtx->BindHeap();
			a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

			auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->staticIndex);
			a_pCtx->SetGraphicPSO(_pPSO);

			auto _main = _spPassData->pRG->GetSRVCPU("QuadTexture");
			auto _ui = _spPassData->pRG->GetSRVCPU("UITexture");

			a_pCtx->ChangeBackBuffer();
			a_pCtx->BindSRV(0, _main);
			a_pCtx->BindSRV(1, _ui);
			a_pCtx->DrawQuad();

			Editor::MainEditor::Instance().EndWatch("FullScreenPass");
		};

		a_rg.AddPassNode(a_phase, _node);
	}
}