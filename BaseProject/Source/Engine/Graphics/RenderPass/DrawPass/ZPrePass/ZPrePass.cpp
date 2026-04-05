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
		SetName("ZPrePass");

		SetInputLayout(D3D12::Input::AnimationInputLayout);
		SetVS("Asset/Shader/Source/ZPreShader/ZPreVS.cso");
		SetRootSig("BaseRootSig");
		m_psoDesc.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		AddWrite("Depth", AccessType::Depth_Write, LoadOp::Clear, StoreOp::Store);
	}
}
