#include "ForwardLightingPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

void ForwardLightingPass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	DrawQueue(a_pCtx,RenderQueueType::Transparent);

	End(a_pCtx);
}

void ForwardLightingPass::CreatePass()
{
	D3D12_INPUT_ELEMENT_DESC _layout[5] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	D3D12_INPUT_LAYOUT_DESC _desc = {
		.pInputElementDescs = _layout,
		.NumElements = 5
	};
	Resource::ID _vsID = m_pShaderMana->Register(
		{"Asset/Shader/Compiled/ForwardLightingShader/ForwardLightingVS.cso", ShaderStage::Vertex,&_desc });
	Resource::ID _psID = m_pShaderMana->Register({
		"Asset/Shader/Compiled/ForwardLightingShader/ForwardLightingPS.cso", ShaderStage::Pixel });

	Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

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

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = _blend;											// ブレンドステートもデフォルト
	_psoDesc.DepthStencilState = _depth;									// 深度ステンシルはデフォルトを使用
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 1;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;				// カラーフォーマット
	_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_psoDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_psoDesc.PS = m_pShaderMana->NGet(_psID)->byteCode;
	_psoDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Resource::ID _psoID = m_pPSOMana->Register("SimplePass", _psoDesc);

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
