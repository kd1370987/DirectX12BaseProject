#include "GBufferPass.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "../../../../D3D12/Builder/RootSignatureBuilder/RootSignatureBuilder.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void GBufferPass::Excute(RenderContext* a_pCtx)
	{
		//Begine(a_pCtx);
		a_pCtx->BindHeap();
		a_pCtx->SetGraphicsRootSignature(m_pRootSig);
		a_pCtx->BindCameraCB();

		DrawQueue(a_pCtx);

		End(a_pCtx);
	}

	void GBufferPass::CreatePass()
	{
		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Opaque);
		auto& _aPso = AddPSODesc(ERenderType::Animation, RenderQueueType::AnimationOpaque);
		_sPso.SetName("GBufferStatic");
		_aPso.SetName("GBufferAnimation");

		SetInputLayout(ERenderType::Static,D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static, "Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		SetInputLayout(ERenderType::Animation, D3D12::Input::AnimationInputLayout);
		SetVS(ERenderType::Animation, "Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso");

		SetPS("Asset/Shader/Source/GBufferShader/GBufferPS.cso");

		// ルートシグネチャ作成
		//m_pRootSig = m_pPipelineStateManager->Request("Asset/Shader/Source/GBufferShader/GBufferVS.cso");
		//SetRootSig(m_pRootSig);


		_sPso.DepthEnable(true);
		_sPso.DepthWriteMask(false);
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		_aPso.DepthEnable(true);
		_aPso.DepthWriteMask(false);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		AddRead("Depth", AccessType::Depth_Read, LoadOp::Load, StoreOp::Store);

		AddWrite("GBufferAlbedo", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		AddWrite("GBufferNormal", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		AddWrite("GBufferMaterial", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		AddWrite("GBufferEmissiv", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}