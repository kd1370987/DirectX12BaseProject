#include "FullScreenPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
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

		Resource::Handle<Resource::Shader> _vsHandle = 
			m_pShaderMana->Request("Asset/Shader/Source/QuadRenderingShader/QuadRenderingVS.cso");
		Resource::Handle<Resource::Shader> _psHandle = 
			m_pShaderMana->Request("Asset/Shader/Source/QuadRenderingShader/QuadRenderingPS.cso");

		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("QuadRendering");

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("FullScreenPass");

		_gPSODesc.DepthEnable(false);
		_gPSODesc.StencilEnable(false);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	

		// 基本情報
		_gPSODesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
		_gPSODesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
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