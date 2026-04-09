#include "FullScreenPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void FullScreenPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		auto _main = m_pRenderGraph->GetGPUHandle("QuadTexture");
		auto _ui = m_pRenderGraph->GetGPUHandle("UITexture");

		a_pCtx->ChangeBackBuffer();
		a_pCtx->BindSRV(RootSigSemantic::PostScreenSRV, { _main });
		a_pCtx->BindSRV(1, { _ui });
		a_pCtx->DrawQuad();

		End(a_pCtx);
	}

	void FullScreenPass::CreatePass()
	{
		SetName("FullScreenPass");

		SetVS("Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso");
		SetPS("Asset/Shader/Source/QuadRenderingShader/QuadRenderingPS.cso");
		SetRootSig("QuadRendering");

		m_psoDesc.DepthEnable(false);
		m_psoDesc.StencilEnable(false);

		AddRead("QuadTexture", AccessType::SRV, LoadOp::Load, StoreOp::DontCare);
		AddRead("UITexture", AccessType::SRV, LoadOp::Load, StoreOp::DontCare);

		// バックバッファは登録されていないため手動で
		m_psoDesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}