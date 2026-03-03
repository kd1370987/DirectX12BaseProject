#include "DebugLinePass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

void DebugLinePass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	DrawQueue(a_pCtx, RenderQueueType::Opaque);
	DrawQueue(a_pCtx, RenderQueueType::AnimationOpaque);
	DrawQueue(a_pCtx, RenderQueueType::Transparent);
	DrawQueue(a_pCtx, RenderQueueType::AnimationTransparent);

	DrawQueue(a_pCtx, RenderQueueType::Debug);

	End(a_pCtx);
}

void DebugLinePass::CreatePass()
{
	// シェーダー
	D3D12_INPUT_ELEMENT_DESC _layout[7] =
	{
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	D3D12_INPUT_LAYOUT_DESC _desc = {
		.pInputElementDescs = _layout,
		.NumElements = 1
	};
	Resource::ID _vsID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/DebugLineShader/DebugLineVS.hlsl", ShaderStage::Vertex ,&_desc });
	Resource::ID _psID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/DebugLineShader/DebugLinePS.hlsl", ShaderStage::Pixel });

	// ルートシグネチャ
	Resource::ID _rootSigID = m_pRootSigMana->GetID("DebugLine");

	// 深度ステンシルステート
	auto _depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用



	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);				// ブレンドステートもデフォルト
	_psoDesc.DepthStencilState = _depthDesc;								// 深度ステンシルはデフォルトを使用
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;	// 線を描画
	_psoDesc.NumRenderTargets = 1;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;				// カラーフォーマット
	_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_psoDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_psoDesc.PS = m_pShaderMana->NGet(_psID)->byteCode;
	_psoDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Resource::ID _psoID = m_pPSOMana->Register("DebugLinePass", _psoDesc);

	// Desc構造体作成
	m_passDesc = {};
	m_passDesc.name = "DebugLinePass";

	m_passDesc.rootSigID = _rootSigID;
	m_passDesc.psoID = _psoID;

	m_passDesc.queueType = RenderQueueType::Opaque;

	auto _mainTexID = m_pRenderGraph->GetID("MainTexture");
	auto _depthTexID = m_pRenderGraph->GetID("Depth");

	// 入力元
	m_passDesc.readResource.push_back(_mainTexID);

	// 出力先
	m_passDesc.writeResource.push_back(_mainTexID);
	m_passDesc.writeResource.push_back(_depthTexID);

	// リソース
	m_passDesc.resourceAccessVec = {
		{_mainTexID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
		{_depthTexID,AccessType::Depth_Write,LoadOp::Clear,StoreOp::Store}
	};
}
