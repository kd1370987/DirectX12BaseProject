#include "FullScreenPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void FullScreenPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		auto _main = m_pRenderGraph->GetGPUHandle("QuadTexture");

		a_pCtx->ChangeBackBuffer();
		a_pCtx->BindSRV(RootSigSemantic::PostScreenSRV, { _main });
		a_pCtx->DrawQuad();

		End(a_pCtx);
	}

	void FullScreenPass::CreatePass()
	{
		//D3D12_INPUT_ELEMENT_DESC _layout[2] =
		//{
		//	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		//	D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//	{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
		//	D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
		//};
		//D3D12_INPUT_LAYOUT_DESC _desc = {
		//	.pInputElementDescs = _layout,
		//	.NumElements = 2
		//};
		//Engine::Resource::ID _vsID = m_pShaderMana->Register({ "Asset/Shader/Compiled/QuadRenderingShader/QuadRenderingVS.cso",ShaderStage::Vertex,&_desc });
		Engine::Resource::ID _vsID = m_pShaderMana->Register({ "Asset/Shader/Compiled/QuadRenderingShader/QuadRenderingVS.cso",ShaderStage::Vertex});
		Engine::Resource::ID _psID = m_pShaderMana->Register({ "Asset/Shader/Compiled/QuadRenderingShader/QuadRenderingPS.cso",ShaderStage::Pixel });

		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("QuadRendering");

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("FullScreenPass");

		_gPSODesc.DepthEnable(false);
		_gPSODesc.StencilEnable(false);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	

		// 基本情報
		_gPSODesc.SetInputLayout(m_pShaderMana->NGet(_vsID)->vsInputLayout);
		_gPSODesc.SetVS(m_pShaderMana->NGet(_vsID)->byteCode);
		_gPSODesc.SetPS(m_pShaderMana->NGet(_psID)->byteCode);
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);
		// Desc構造体作成
		m_passDesc = {};
		m_passDesc.name = "FullScreenPass";

		m_passDesc.rootSigID = _rootSigID;
		m_passDesc.psoID = _psoID;

		auto _id = m_pRenderGraph->GetID("QuadTexture");

		// 入力元
		m_passDesc.readResource.push_back(_id);

		// 出力先

		// リソース
		m_passDesc.resourceAccessVec = {
			{_id,AccessType::SRV,LoadOp::Load,StoreOp::DontCare}
		};
	}
}