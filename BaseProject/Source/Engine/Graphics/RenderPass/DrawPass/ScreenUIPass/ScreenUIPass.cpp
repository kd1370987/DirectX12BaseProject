#include "ScreenUIPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void ScreenUIPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		a_pCtx->DrawUIQueue(RenderQueueType2D::ScreenUI);

		End(a_pCtx);
	}

	void ScreenUIPass::CreatePass()
	{
		// 頂点レイアウト
		D3D12_INPUT_ELEMENT_DESC _layout[2] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		D3D12_INPUT_LAYOUT_DESC _desc = {
			.pInputElementDescs = _layout,
			.NumElements = 2
		};

		// シェーダー登録
		Engine::Resource::ID _vsID = m_pShaderMana->Register(
			{ "Asset/Shader/Compiled/Screen2DShader/Screen2DVS.cso", ShaderStage::Vertex ,&_desc });
		Engine::Resource::ID _psID = m_pShaderMana->Register(
			{ "Asset/Shader/Compiled/Screen2DShader/Screen2DPS.cso", ShaderStage::Pixel });

		// ルートシグネチャ
		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("2DRootSig");

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

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("ScreenUIPass");

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_NONE);
		_gPSODesc.SetBlendState(_blendDesc);
		_gPSODesc.SetDepthStencilState(_depthDesc);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		

		// 基本情報
		_gPSODesc.SetInputLayout(m_pShaderMana->NGet(_vsID)->vsInputLayout);
		_gPSODesc.SetVS(m_pShaderMana->NGet(_vsID)->byteCode);
		_gPSODesc.SetPS(m_pShaderMana->NGet(_psID)->byteCode);
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);

		// Desc構造体作成
		m_passDesc = {};
		m_passDesc.name = "ScreenUIPass";

		m_passDesc.rootSigID = _rootSigID;
		m_passDesc.psoID = _psoID;

		auto _tex = m_pRenderGraph->GetID("QuadTexture");

		// 入力元
		m_passDesc.readResource.push_back(_tex);

		// 出力先
		m_passDesc.writeResource.push_back(_tex);

		// リソース
		m_passDesc.resourceAccessVec = {
			{_tex,AccessType::RTV,LoadOp::Load,StoreOp::Store},
		};
	}
}