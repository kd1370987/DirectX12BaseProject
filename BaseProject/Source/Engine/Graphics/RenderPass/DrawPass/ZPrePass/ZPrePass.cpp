#include "ZPrePass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

void ZPrePass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	DrawQueue(a_pCtx, RenderQueueType::Opaque);
	DrawQueue(a_pCtx, RenderQueueType::AnimationOpaque);
	//DrawQueue(a_pCtx, RenderQueueType::Transparent);

	End(a_pCtx);
}

void ZPrePass::CreatePass()
{
	// シェーダー
	D3D12_INPUT_ELEMENT_DESC _layout[7] =
	{
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SKININDEX",  0, DXGI_FORMAT_R16G16B16A16_UINT,	0, D3D12_APPEND_ALIGNED_ELEMENT,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "SKINWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	D3D12_INPUT_LAYOUT_DESC _desc = {
		.pInputElementDescs = _layout,
		.NumElements = 7
	};
	Resource::ID _vsID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/ZPreShader/ZPreVS.cso", ShaderStage::Vertex,&_desc }
	);

	// ルートシグネチャ
	Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;				// カリングなし
	_psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);				// ブレンドステートもデフォルト
	auto _ds = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);					// 深度ステンシルはデフォルトを使用
	_ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	_psoDesc.DepthStencilState = _ds;
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 0;											// 描画対象数
	_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_psoDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_psoDesc.PS = {nullptr,0};
	_psoDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Resource::ID _psoID = m_pPSOMana->Register("ZPrePass", _psoDesc);

	// パス情報
	m_passDesc = {};
	m_passDesc.name = "ZPrePass";

	m_passDesc.rootSigID = _rootSigID;
	m_passDesc.psoID = _psoID;

	auto _depth = m_pRenderGraph->GetID("Depth");

	// 入力元
	
	// 出力元
	m_passDesc.writeResource.push_back(_depth);

	// リソース
	m_passDesc.resourceAccessVec = {
		{_depth,AccessType::Depth_Write,LoadOp::Clear,StoreOp::Store}
	};
}
