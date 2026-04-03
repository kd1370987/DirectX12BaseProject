#include "DebugLinePass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void DebugLinePass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		a_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		auto& _draws = a_pCtx->GetItemVec(RenderQueueType::Debug);
		if (_draws.size() == 0) return;
		for (auto& _item : _draws)
		{
			a_pCtx->BindMesh(_item.pMesh, _item.worldMat);

			a_pCtx->Draw(_item.pMesh, _item.subIdx);
		}

		a_pCtx->ShapeDraw();

		End(a_pCtx);
	}

	void DebugLinePass::CreatePass()
	{
		// シェーダー
		D3D12_INPUT_ELEMENT_DESC _layout[7] =
		{
			{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		D3D12_INPUT_LAYOUT_DESC _desc = {
			.pInputElementDescs = _layout,
			.NumElements = 1
		};
		Engine::Resource::ID _vsID = m_pShaderMana->Register(
			{ "Asset/Shader/Compiled/DebugLineShader/DebugLineVS.cso", ShaderStage::Vertex ,&_desc });
		Engine::Resource::ID _psID = m_pShaderMana->Register(
			{ "Asset/Shader/Compiled/DebugLineShader/DebugLinePS.cso", ShaderStage::Pixel });

		// ルートシグネチャ
		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("DebugLine");

		// 深度ステンシルステート
		auto _depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用
		_depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("DebugLinePass");
		_gPSODesc.desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_NONE);
		_gPSODesc.FillMode(D3D12_FILL_MODE_WIREFRAME);

		_gPSODesc.SetDepthStencilState(_depthDesc);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

		// 基本情報
		_gPSODesc.SetInputLayout(m_pShaderMana->NGet(_vsID)->vsInputLayout);
		_gPSODesc.SetVS(m_pShaderMana->NGet(_vsID)->byteCode);
		_gPSODesc.SetPS(m_pShaderMana->NGet(_psID)->byteCode);
		_gPSODesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));

		// リクエスト
		Resource::Handle<D3D12::PipelineState> _psoID = m_pPSOMana->Request(_gPSODesc);

		// Desc構造体作成
		m_passDesc = {};
		m_passDesc.name = "DebugLinePass";

		m_passDesc.rootSigID = _rootSigID;
		m_passDesc.psoID = _psoID;

		m_passDesc.queueType = RenderQueueType::Opaque;

		auto _mainTexID = m_pRenderGraph->GetID("MainColor");
		//auto _mainTexID = m_pRenderGraph->GetID("QuadTexture");
		auto _depthTexID = m_pRenderGraph->GetID("Depth");

		// 入力元
		m_passDesc.readResource.push_back(_mainTexID);
		m_passDesc.readResource.push_back(_depthTexID);

		// 出力先
		m_passDesc.writeResource.push_back(_mainTexID);
		//m_passDesc.writeResource.push_back(_depthTexID);

		// リソース
		m_passDesc.resourceAccessVec = {
			{_mainTexID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
			{_depthTexID,AccessType::Depth_Write,LoadOp::Load,StoreOp::Store}
		};
	}
}