#include "GBufferPass.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
namespace Engine::Graphics
{
	void GBufferPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		DrawQueue(a_pCtx, RenderQueueType::Opaque);

		End(a_pCtx);
	}

	void GBufferPass::CreatePass()
	{
		D3D12_INPUT_ELEMENT_DESC _layout[5] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		D3D12_INPUT_LAYOUT_DESC _desc = {
			.pInputElementDescs = _layout,
			.NumElements = 5
		};
		Engine::Resource::ID _vsID = m_pShaderMana->Register(
			{ "Asset/Shader/Compiled/GBufferShader/GBufferVS.cso", ShaderStage::Vertex ,&_desc });
		Engine::Resource::ID _psID = m_pShaderMana->Register(
			{ "Asset/Shader/Compiled/GBufferShader/GBufferPS.cso", ShaderStage::Pixel });

		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID("BaseRootSig");

		// パイプラインステート初期化
		D3D12::GraphicsPipelineDesc _gPSODesc = {};
		_gPSODesc.SetName("GBufferPass");

		// ラスタライザ
		_gPSODesc.CullMode(D3D12_CULL_MODE_NONE);

		// 描画先
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R16G16_FLOAT);
		_gPSODesc.AddRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
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
		m_passDesc.name = "GBufferPass";

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
		//m_passDesc.writeResource.push_back(_depth);			// ZPreの後でもこれ必要
		m_passDesc.writeResource.push_back(_gbAlbedoID);
		m_passDesc.writeResource.push_back(_gbNormalID);
		m_passDesc.writeResource.push_back(_gbMaterialID);
		m_passDesc.writeResource.push_back(_gbEmiID);

		// リソース
		m_passDesc.resourceAccessVec = {
			{_gbAlbedoID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
			{_gbNormalID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
			{_gbMaterialID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
			{_gbEmiID,AccessType::RTV,LoadOp::Clear,StoreOp::Store},
			{_depth,AccessType::Depth_Write,LoadOp::Load,StoreOp::Store}
		};
	}
}