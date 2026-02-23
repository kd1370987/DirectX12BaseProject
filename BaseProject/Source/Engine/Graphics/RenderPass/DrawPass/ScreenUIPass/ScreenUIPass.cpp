#include "ScreenUIPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

void ScreenUIPass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	

	End(a_pCtx);
}

void ScreenUIPass::CreatePass()
{
	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC _layout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_INPUT_LAYOUT_DESC _desc = {
		.pInputElementDescs = _layout,
		.NumElements = 2
	};

	// シェーダー登録
	Resource::ID _vsID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/Screen2DShader/Screen2DVS.cso", ShaderStage::Vertex ,&_desc });
	Resource::ID _psID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/Screen2DShader/Screen2DPS.cso", ShaderStage::Pixel });

	// ルートシグネチャ
	Resource::ID _rootSigID = m_pRootSigMana->GetID("2DRootSig");

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

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = _blendDesc;										// ブレンドステート
	_psoDesc.DepthStencilState = _depthDesc;								// 深度ステンシル
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 1;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;				// カラーフォーマット
	_psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;								// 深度フォーマット
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_psoDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_psoDesc.PS = m_pShaderMana->NGet(_psID)->byteCode;
	_psoDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Resource::ID _psoID = m_pPSOMana->Register("ScreenUIPass", _psoDesc);

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
