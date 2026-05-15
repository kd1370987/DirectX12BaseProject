#include "ForwardLightingPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void ForwardLightingPass::Excute(RenderContext* a_pCtx)
	{

		Begine(a_pCtx);
		a_pCtx->BindCameraCB();
		a_pCtx->BindSRVBone();

		DrawQueue(a_pCtx);

		End(a_pCtx);
	}

	void ForwardLightingPass::CreatePass()
	{
		// ブレンドステート
		D3D12_BLEND_DESC _blend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		_blend.RenderTarget[0].BlendEnable = TRUE;
		_blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		_blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		_blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		_blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		_blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		_blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		_blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// 深度
		D3D12_DEPTH_STENCIL_DESC _depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		_depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 深度値書き込みなし
		_depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Transparent);
		_sPso.SetName("ForwardLithingPSO");

		SetInputLayout(ERenderType::Static,D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static,"Asset/Shader/Source/ForwardLightingShader/ForwardLightingVS.cso");
		SetPS("Asset/Shader/Source/ForwardLightingShader/ForwardLightingPS.cso");
		
		_sPso.SetDepthStencilState(_depth);
		_sPso.SetBlendState(_blend);

		AddRead("Depth",AccessType::Depth_Read, LoadOp::Load, StoreOp::DontCare);
		AddRead("QuadTexture");
		AddWrite("QuadTexture", AccessType::RTV, LoadOp::Load, StoreOp::Store);
	}
}