#include "GBufferPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

void GBufferPass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	DrawQueue(a_pCtx,RenderQueueType::Opaque);

	End(a_pCtx);
}

void GBufferPass::CreatePass()
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
		{ "Asset/Shader/Compiled/GBufferShader/GBufferVS.cso", ShaderStage::Vertex ,&_desc });
	Resource::ID _psID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/GBufferShader/GBufferPS.cso", ShaderStage::Pixel });

	Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

	// 深度ステンシルステート
	auto _depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);				// ブレンドステートもデフォルト
	_psoDesc.DepthStencilState = _depthDesc;								// 深度ステンシルはデフォルトを使用
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 4;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;				// カラーフォーマット
	_psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_FLOAT;						// 法線
	_psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;					// マテリアル
	_psoDesc.RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM;					// エミッシブ
	_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	//_psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;								// 深度フォーマット（Zバッファの精度）
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_psoDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_psoDesc.PS = m_pShaderMana->NGet(_psID)->byteCode;
	_psoDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Resource::ID _psoID = m_pPSOMana->Register("GBufferPass", _psoDesc);

	// Desc構造体作成
	m_passDesc = {};
	m_passDesc.name = "GBufferPass";

	m_passDesc.rootSigID = _rootSigID;
	m_passDesc.psoID = _psoID;

	m_passDesc.queueType = RenderQueueType::Opaque;

	auto _depth = m_pRenderGraph->GetID("Depth");
	auto _gbAlbedoID = m_pRenderGraph->GetID("GBufferAlbedo");
	auto _gbNormalID = m_pRenderGraph->GetID("GBufferNormal");
	auto _gbMaterialID = m_pRenderGraph->GetID("GBufferMaterial");
	auto _gbEmiID = m_pRenderGraph->GetID("GBufferEmissiv");

	// 入力元

	// 出力先
	m_passDesc.writeResource.push_back(_depth);			// ZPreの後でもこれ必要
	m_passDesc.writeResource.push_back(_gbAlbedoID);
	m_passDesc.writeResource.push_back(_gbNormalID);
	m_passDesc.writeResource.push_back(_gbMaterialID);
	m_passDesc.writeResource.push_back(_gbEmiID);

	// リソース
	m_passDesc.resourceAccessVec = {
		{_gbAlbedoID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
		{_gbNormalID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
		{_gbMaterialID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
		{_gbEmiID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
		{_depth,AccessType::Depth_Write,LoadOp::Load,StoreOp::Store}
	};
}
