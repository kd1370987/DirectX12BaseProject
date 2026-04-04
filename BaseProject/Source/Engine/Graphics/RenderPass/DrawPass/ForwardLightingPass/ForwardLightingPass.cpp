#include "ForwardLightingPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
namespace Engine::Graphics
{
	void ForwardLightingPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		DrawQueue(a_pCtx, RenderQueueType::Transparent);

		End(a_pCtx);
	}

	void ForwardLightingPass::CreatePass()
	{
		// シェーダー登録
		Resource::Handle<Resource::Shader> _vsHandle = 
			m_pShaderMana->Request("Asset/Shader/Source/ForwardLightingShader/ForwardLightingVS.cso");
		Resource::Handle<Resource::Shader> _psHandle =
			m_pShaderMana->Request("Asset/Shader/Source/ForwardLightingShader/ForwardLightingPS.cso");

		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("ForwardLithingPass");

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
		_depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		_depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("ForwardLithingPSO");

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_NONE);

		_gPSODesc.SetBlendState(_blend);

		_gPSODesc.SetDepthStencilState(_depth);
		_gPSODesc.desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);


		// 基本情報
		_gPSODesc.SetInputLayout(D3D12::Input::StaticLayout);
		_gPSODesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
		_gPSODesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);

		// Desc構造体作成
		m_passDesc = {};
		m_passDesc.name = "ForwardLightingPass";

		m_passDesc.rootSigID = _rootSigID;
		m_passDesc.psoID = _psoID;

		auto _depthRes = m_pRenderGraph->GetID("Depth");
		auto _mainColorID = m_pRenderGraph->GetID("QuadTexture");

		// 入力元
		m_passDesc.readResource.push_back(_depthRes);
		m_passDesc.readResource.push_back(_mainColorID);

		m_passDesc.writeResource.push_back(_mainColorID);

		// リソース
		m_passDesc.resourceAccessVec = {
			{_mainColorID,AccessType::RTV,LoadOp::Load,StoreOp::Store},
			{_depthRes,AccessType::Depth_Write,LoadOp::Load,StoreOp::DontCare}
		};
	}
}