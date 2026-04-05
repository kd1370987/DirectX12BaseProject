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

		SetName("AnimationGBufferPass");

		SetInputLayout(D3D12::Input::AnimationInputLayout);
		SetVS("Asset/Shader/Source/GBufferShader/AnimationGBufferVS.cso");
		SetPS("Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		SetRootSig("BaseRootSig");

		m_psoDesc.DepthEnable(true);
		m_psoDesc.DepthWriteMask(false);
		m_psoDesc.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		AddRead("Depth", AccessType::Depth_Read, LoadOp::Load, StoreOp::Store);

		AddWrite("GBufferAlbedo", AccessType::RTV, LoadOp::Load, StoreOp::Store);
		AddWrite("GBufferNormal", AccessType::RTV, LoadOp::Load, StoreOp::Store);
		AddWrite("GBufferMaterial", AccessType::RTV, LoadOp::Load, StoreOp::Store);
		AddWrite("GBufferEmissiv", AccessType::RTV, LoadOp::Load, StoreOp::Store);
	}
}