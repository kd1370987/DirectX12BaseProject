#include "ZPrePass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
namespace Engine::Graphics
{
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
		Resource::Handle<Resource::Shader> _vsHandle = 
			m_pShaderMana->Request("Asset/Shader/Compiled/ZPreShader/ZPreVS.cso");

		// ルートシグネチャ
		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

		auto _ds = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);					// 深度ステンシルはデフォルトを使用
		_ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		
		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("ZPrePass");

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_BACK);
		_gPSODesc.SetDepthStencilState(_ds);

		// 基本情報
		_gPSODesc.SetInputLayout(D3D12::Input::AnimationInputLayout);
		_gPSODesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);

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
}
