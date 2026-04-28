#include "AnimationZPrePass.h"

namespace Engine::Graphics
{
	void Engine::Graphics::AnimationZPrePass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		DrawQueue(a_pCtx,RenderQueueType::AnimationOpaque);

		End(a_pCtx);
	}
	void AnimationZPrePass::CreatePass()
	{
		SetName("AnimationZPrePass");

		SetInputLayout(D3D12::Input::AnimationInputLayout);
		SetVS("Asset/Shader/Source/ZPreShader/AnimationZPreVS.cso");
		SetRootSig("BaseRootSig");
		m_psoDesc.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		AddRead("Depth");
		AddWrite("Depth", AccessType::Depth_Write, LoadOp::Load, StoreOp::Store);
	}
}