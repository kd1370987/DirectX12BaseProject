#include "AnimationGBufferPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
namespace Engine::Graphics
{
	void AnimationGBufferPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		DrawQueue(a_pCtx, RenderQueueType::AnimationOpaque);

		End(a_pCtx);
	}

	void AnimationGBufferPass::CreatePass()
	{
		Resource::Handle<Resource::Shader> _vsHandle = 
			m_pShaderMana->Request("Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso");
		Resource::Handle<Resource::Shader> _psHandle = 
			m_pShaderMana->Request("Asset/Shader/Source/GBufferShader/GBufferPS.cso");

		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("AnimationGBufferPass");

		_gPSODesc.DepthEnable(true);
		_gPSODesc.DepthWriteMask(false);
		_gPSODesc.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_BACK);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R16G16_FLOAT);
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

		// 基本情報
		_gPSODesc.SetInputLayout(D3D12::Input::AnimationInputLayout);
		_gPSODesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
		_gPSODesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);


		// Desc構造体作成
		m_passDesc = {};
		m_passDesc.name = "AnimationGBufferPass";

		m_passDesc.rootSigID = _rootSigID;
		m_passDesc.psoID = _psoID;

		m_passDesc.queueType = RenderQueueType::Opaque;

		auto _depth = m_pRenderGraph->GetID("Depth");
		auto _gbAlbedoID = m_pRenderGraph->GetID("GBufferAlbedo");
		auto _gbNormalID = m_pRenderGraph->GetID("GBufferNormal");
		auto _gbMaterialID = m_pRenderGraph->GetID("GBufferMaterial");
		auto _gbEmiID = m_pRenderGraph->GetID("GBufferEmissiv");

		// 入力元
		m_passDesc.readResource.push_back(_depth);

		// 出力先
		m_passDesc.writeResource.push_back(_gbAlbedoID);
		m_passDesc.writeResource.push_back(_gbNormalID);
		m_passDesc.writeResource.push_back(_gbMaterialID);
		m_passDesc.writeResource.push_back(_gbEmiID);

		// リソース
		m_passDesc.resourceAccessVec = {
			{_gbAlbedoID,AccessType::RTV,LoadOp::Load,StoreOp::Store},
			{_gbNormalID,AccessType::RTV,LoadOp::Load,StoreOp::Store},
			{_gbMaterialID,AccessType::RTV,LoadOp::Load,StoreOp::Store},
			{_gbEmiID,AccessType::RTV,LoadOp::Load,StoreOp::Store},
			{_depth,AccessType::Depth_Read,LoadOp::Load,StoreOp::Store}
		};
	}
}