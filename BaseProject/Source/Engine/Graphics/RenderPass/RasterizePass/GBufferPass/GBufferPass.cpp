#include "GBufferPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../GraphicEngine.h"

#include "../../../../D3D12/Builder/RootSignatureBuilder/RootSignatureBuilder.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../../D3D12/CBAllocater/CBAllocater.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void GBufferPass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		Begine(a_pCtx);
		a_pCtx->BindRootCBV<CameraData>(0,a_pGE->GetCameraData());
		a_pCtx->BindInstanceBuffer(2);
		a_pCtx->BindSubsetBuffer(3);
		a_pCtx->BindBonePalletBuffer(4);

		DrawQueue(a_pGE,a_pCtx);

		End(a_pCtx);
	}

	void GBufferPass::CreatePass()
	{
		SetPassName("GBuffer");
		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Opaque);
		auto& _aPso = AddPSODesc(ERenderType::Animation, RenderQueueType::AnimationOpaque);
		_sPso.SetName("GBufferStatic");
		_aPso.SetName("GBufferAnimation");

		SetInputLayout(ERenderType::Static,D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static, "Asset/Shader/Source/GBufferShader/GBufferVS.cso");

		SetInputLayout(ERenderType::Animation, D3D12::Input::AnimationInputLayout);
		SetVS(ERenderType::Animation, "Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso");

		SetPS("Asset/Shader/Source/GBufferShader/GBufferPS.cso");

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
		AddWrite("GBufferVelocity", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}