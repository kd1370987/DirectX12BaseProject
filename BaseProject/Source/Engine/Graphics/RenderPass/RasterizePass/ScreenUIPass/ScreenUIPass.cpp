#include "ScreenUIPass.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void ScreenUIPass::Excute(RenderContext* a_pCtx)
	{
		Begine(a_pCtx);

		a_pCtx->SetGraphicPSO(m_pPsoVec[0].first);

		a_pCtx->DrawUIQueue(RenderQueueType2D::ScreenUI);

		End(a_pCtx);
	}

	void ScreenUIPass::CreatePass()
	{
		// 深度ステンシルステート
		auto _depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用
		_depthDesc.DepthEnable = FALSE;									// 深度テストなし
		_depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;		// 深度書き込みなし

		// ブレンドステート
		auto _blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		_blendDesc.RenderTarget[0].BlendEnable = TRUE;					// ブレンド有効
		_blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;	// ソースのブレンド係数
		_blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;// デストのブレンド係数
		_blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;		// ブレンドの演算方法
		_blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;	// ソースのアルファブレンド係数
		_blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;// デストのアルファブレンド係数
		_blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;	// アルファブレンドの演算方法

		auto& _psoDesc = AddPSODesc(ERenderType::Static, RenderQueueType::Simple);
		_psoDesc.SetName("ScreenUI");
		

		SetInputLayout(ERenderType::Static,D3D12::Input::Static2DLayout);
		SetVS(ERenderType::Static,"Asset/Shader/Source/Screen2DShader/Screen2DVS.cso");
		SetPS("Asset/Shader/Source/Screen2DShader/Screen2DPS.cso");

		_psoDesc.SetDepthStencilState(_depthDesc);
		_psoDesc.SetBlendState(_blendDesc);

		AddWrite("UITexture", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}