#include "GBufferPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
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
		SetName("GBufferPass");

		SetInputLayout(D3D12::Input::StaticLayout);
		SetVS("Asset/Shader/Source/GBufferShader/GBufferVS.cso");
		SetPS("Asset/Shader/Source/GBufferShader/GBufferPS.cso");
		SetRootSig("BaseRootSig");

		AddRead("Depth", AccessType::Depth_Write, LoadOp::Load, StoreOp::Store);

		AddWrite("GBufferAlbedo", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		AddWrite("GBufferNormal", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		AddWrite("GBufferMaterial", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
		AddWrite("GBufferEmissiv", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}