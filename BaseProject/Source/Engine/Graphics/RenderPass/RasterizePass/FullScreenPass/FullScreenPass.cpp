#include "FullScreenPass.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void FullScreenPass::Excute(RenderContext* a_pCtx)
	{
		Begine(a_pCtx);

		a_pCtx->SetGraphicPSO(m_pPsoVec[0].first);
		auto _main = m_pRG->GetCPUHandle("QuadTexture");
		auto _ui = m_pRG->GetCPUHandle("UITexture");

		a_pCtx->ChangeBackBuffer();
		a_pCtx->BindSRV(0, _main);
		a_pCtx->BindSRV(1, _ui);
		a_pCtx->DrawQuad();

		End(a_pCtx);
	}

	void FullScreenPass::CreatePass()
	{

		auto& _sPso = AddPSODesc(ERenderType::Static,RenderQueueType::Bloom);
		_sPso.SetName("FullScreenPass");

		SetVS(ERenderType::Static,"Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso");
		SetPS("Asset/Shader/Source/QuadRenderingShader/QuadRenderingPS.cso");
		
		_sPso.DepthEnable(false);
		_sPso.StencilEnable(false);

		AddRead("QuadTexture", AccessType::SRV, LoadOp::Load, StoreOp::DontCare);
		AddRead("UITexture", AccessType::SRV, LoadOp::Load, StoreOp::DontCare);

		// バックバッファは登録されていないため手動で
		_sPso.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}