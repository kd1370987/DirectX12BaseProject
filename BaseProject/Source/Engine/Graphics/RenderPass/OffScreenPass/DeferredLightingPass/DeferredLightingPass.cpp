#include "DeferredLightingPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::Graphics
{
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
		Resource::Handle<Resource::Shader> _vsHandle =
			m_pShaderMana->Request("Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso");
		Resource::Handle<Resource::Shader> _psHandle = 
			m_pShaderMana->Request("Asset/Shader/Source/DeferredLightingShader/DeferredLightingPS.cso");

		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("DeferredLighting");

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("DeferredLighting");

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_NONE);
		// 深度テスト・深度書き込みなし
		_gPSODesc.DepthEnable(false);
		_gPSODesc.DepthWriteMask(false);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

		// 基本情報
		_gPSODesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
		_gPSODesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);

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
}