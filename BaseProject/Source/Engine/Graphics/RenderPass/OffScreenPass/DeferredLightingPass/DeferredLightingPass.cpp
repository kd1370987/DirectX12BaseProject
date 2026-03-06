#include "DeferredLightingPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

void DeferredLightingPass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);

	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> _gpuVec = {};

	_gpuVec = { 
		m_pRenderGraph->GetGPUHandle("GBufferAlbedo"),
		m_pRenderGraph->GetGPUHandle("GBufferNormal"),
		m_pRenderGraph->GetGPUHandle("GBufferMaterial"),
		m_pRenderGraph->GetGPUHandle("GBufferEmissiv"),
		m_pRenderGraph->GetGPUHandle("Depth")
	};

	a_pCtx->BindSRV(
		RootSigSemantic::PostScreenSRV,
		_gpuVec
	);

	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// 描画
	_pCmdList->DrawInstanced(
		3, 1, 0, 0
	);

	End(a_pCtx);
}

void DeferredLightingPass::CreatePass()
{
	Engine::Resource::ID _vsID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/DeferredLightingShader/DeferredLightingVS.cso",ShaderStage::Vertex }
	);
	Engine::Resource::ID _psID = m_pShaderMana->Register(
		{ "Asset/Shader/Compiled/DeferredLightingShader/DeferredLightingPS.cso",ShaderStage::Pixel }
	);

	Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("DeferredLighting");

	// 深度ステンシルステート
	auto _depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用
	_depthDesc.DepthEnable = FALSE;									// 深度テストなし
	_depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;		// 深度書き込みなし


	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);				// ブレンドステートもデフォルト
	_psoDesc.DepthStencilState = _depthDesc;	// 深度ステンシルはデフォルトを使用
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 1;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;				// カラーフォーマット
	//_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	_psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;								// 深度フォーマット
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_psoDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_psoDesc.PS = m_pShaderMana->NGet(_psID)->byteCode;
	_psoDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Engine::Resource::ID _psoID = m_pPSOMana->Register("DeferredLightingPass", _psoDesc);

	// Desc構造体作成
	m_passDesc = {};
	m_passDesc.name = "DeferredLightingPass";

	m_passDesc.rootSigID = _rootSigID;
	m_passDesc.psoID = _psoID;

	auto _depth = m_pRenderGraph->GetID("Depth");
	auto _gbAlbedoID = m_pRenderGraph->GetID("GBufferAlbedo");
	auto _gbNormalID = m_pRenderGraph->GetID("GBufferNormal");
	auto _gbMaterialID = m_pRenderGraph->GetID("GBufferMaterial");
	auto _gbEmiID = m_pRenderGraph->GetID("GBufferEmissiv");

	auto _quadID = m_pRenderGraph->GetID("QuadTexture");

	// 入力元
	m_passDesc.readResource.push_back(_depth);
	m_passDesc.readResource.push_back(_gbAlbedoID);
	m_passDesc.readResource.push_back(_gbNormalID);
	m_passDesc.readResource.push_back(_gbMaterialID);
	m_passDesc.readResource.push_back(_gbEmiID);

	// 出力先
	m_passDesc.writeResource.push_back(_quadID);

	// リソース
	m_passDesc.resourceAccessVec = {
		{_gbAlbedoID,AccessType::SRV,LoadOp::Load,StoreOp::Store},
		{_gbNormalID,AccessType::SRV,LoadOp::Load,StoreOp::Store},
		{_gbMaterialID,AccessType::SRV,LoadOp::Load,StoreOp::Store},
		{_gbEmiID,AccessType::SRV,LoadOp::Load,StoreOp::Store},
		{_depth,AccessType::SRV,LoadOp::Load,StoreOp::Store},

		{_quadID,AccessType::RTV,LoadOp::Clear,StoreOp::Store}
	};
}
