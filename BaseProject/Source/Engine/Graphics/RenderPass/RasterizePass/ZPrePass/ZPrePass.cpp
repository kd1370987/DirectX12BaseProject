#include "ZPrePass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../GraphicEngine.h"


#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void ZPrePass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		Begine(a_pCtx);
		a_pCtx->BindRootCBV<CameraData>(0, a_pGE->GetCameraData());
		a_pCtx->BindInstanceBuffer(2);
		a_pCtx->BindSubsetBuffer(3);
		a_pCtx->BindBonePalletBuffer(4);
		DrawQueue(a_pGE,a_pCtx);

		End(a_pCtx);
	}

	void ZPrePass::CreatePass()
	{
		SetPassName("ZPre");

		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Opaque);
		auto& _aPso = AddPSODesc(ERenderType::Animation, RenderQueueType::AnimationOpaque);

		_sPso.SetName("ZPreStatic");
		_aPso.SetName("ZPreAnimation");

		SetInputLayout(ERenderType::Static,D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static,"Asset/Shader/Source/ZPreShader/ZPreVS.cso");

		SetInputLayout(ERenderType::Animation, D3D12::Input::AnimationInputLayout);
		SetVS(ERenderType::Animation, "Asset/Shader/Source/ZPreShader/AnimationZPreVS.cso");
		
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		AddWrite("Depth", AccessType::Depth_Write, LoadOp::Clear, StoreOp::Store);
	}
}
