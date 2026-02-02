#include "FullScreenPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

void FullScreenPass::Excute(RenderContext* a_pCtx)
{
	Begin(a_pCtx);


	auto _main = m_pRenderGraph->GetGPUHandle("MainColor");
	//auto _main = m_pRenderGraph->GetGPUHandle("QuadTexture");
	a_pCtx->DrawQuad(_main);

	End(a_pCtx);
}

void FullScreenPass::CreatePass()
{
	D3D12_INPUT_ELEMENT_DESC _layout[2] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
	D3D12_INPUT_LAYOUT_DESC _desc = {
		.pInputElementDescs = _layout,
		.NumElements = 2
	};
	Resource::ID _vsID = m_pShaderMana->Register({ "Asset/Shader/Compiled/QuadRenderingShader/QuadRenderingVS.cso",ShaderStage::Vertex,&_desc });
	Resource::ID _psID = m_pShaderMana->Register({ "Asset/Shader/Compiled/QuadRenderingShader/QuadRenderingPS.cso",ShaderStage::Pixel });

	Resource::ID _rootSigID = m_pRootSigMana->GetID("QuadRendering");

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _gpsDesc = {};
	_gpsDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	_gpsDesc.DepthStencilState.DepthEnable = false;
	_gpsDesc.DepthStencilState.StencilEnable = false;
	_gpsDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	_gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	_gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_gpsDesc.NumRenderTargets = 1;
	_gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	_gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	_gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	_gpsDesc.SampleDesc.Count = 1;
	_gpsDesc.SampleDesc.Quality = 0;
	_gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	_gpsDesc.InputLayout = m_pShaderMana->NGet(_vsID)->vsInputLayout;
	_gpsDesc.VS = m_pShaderMana->NGet(_vsID)->byteCode;
	_gpsDesc.PS = m_pShaderMana->NGet(_psID)->byteCode;
	_gpsDesc.pRootSignature = m_pRootSigMana->NGet(_rootSigID);
	Resource::ID _psoID = m_pPSOMana->Register("FullScreenPass", _gpsDesc);

	// Desc構造体作成
	m_passDesc = {};
	m_passDesc.name = "FullScreenPass";

	m_passDesc.rootSigID = _rootSigID;
	m_passDesc.psoID = _psoID;

	// 入力元
	m_passDesc.readResource.push_back(m_pRenderGraph->GetID("MainColor"));

	// 出力先
	/*auto _quadID = m_pRenderGraph->GetID("QuadTexture");
	m_passDesc.writeResource.push_back(
		_quadID
	);*/

	m_passDesc.queueType = RenderQueueType::Opaque;

	m_passDesc.colorAttachements = {
		//{_quadID,LoadOp::Clear,StoreOp::Store}
	};
	m_passDesc.depthAttachement = {};
}
