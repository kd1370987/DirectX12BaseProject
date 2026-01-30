#include "ForwardLightingPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

void ForwardLightingPass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	DrawQueue(a_pCtx,RenderQueueType::Opaque);
	DrawQueue(a_pCtx,RenderQueueType::Transparent);

	End(a_pCtx);
}

void ForwardLightingPass::CreatePass()
{
	Resource::ID _vsID = m_pShaderMana->Register({ "Asset/Shader/Compiled/SimpleShader/SimpleVS.cso", ShaderStage::Vertex });
	Resource::ID _psID = m_pShaderMana->Register({ "Asset/Shader/Compiled/SimpleShader/SimplePS.cso", ShaderStage::Pixel });

	Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);				// ブレンドステートもデフォルト
	_psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 1;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;				// カラーフォーマット
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

	// 入力元
	auto _depth = m_pRenderGraph->GetID("Depth");
	m_passDesc.readResource.push_back(
		_depth
	);
	
	// 出力先
	auto _mainColorID = m_pRenderGraph->GetID("MainColor");
	m_passDesc.writeResource.push_back(
		_mainColorID
	);

	m_passDesc.queueType = RenderQueueType::Opaque;

	m_passDesc.colorAttachements = {
		{_mainColorID,LoadOp::Clear,StoreOp::Store}
	};
	m_passDesc.depthAttachement = {
		{_depth,LoadOp::Clear,StoreOp::DontCare}
	};
}
