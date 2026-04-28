#include "ZPrePass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
namespace Engine::Graphics
{
	void ZPrePass::Excute(RenderContext* a_pCtx)
	{
		Begine(a_pCtx);

		DrawQueue(a_pCtx);

		End(a_pCtx);
	}

	void ZPrePass::CreatePass()
	{
		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Opaque);
		auto& _aPso = AddPSODesc(ERenderType::Animation, RenderQueueType::AnimationOpaque);

		_sPso.SetName("ZPreStatic");
		_aPso.SetName("ZPreAnimation");

		SetRootSig("BaseRootSig");

		SetInputLayout(ERenderType::Static,D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static,"Asset/Shader/Source/ZPreShader/ZPreVS.cso");

		SetInputLayout(ERenderType::Animation, D3D12::Input::AnimationInputLayout);
		SetVS(ERenderType::Animation, "Asset/Shader/Source/ZPreShader/AnimationZPreVS.cso");
		
		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);
		_aPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		AddWrite("Depth", AccessType::Depth_Write, LoadOp::Clear, StoreOp::Store);
	}
}
