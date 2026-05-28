#include "TestPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"


namespace Engine::Graphics
{
	void Engine::Graphics::TestPass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		Begine(a_pCtx);
		a_pCtx->BindRootCBV<CameraData>(0, a_pGE->GetCameraData());
		End(a_pCtx);
	}
	void TestPass::CreatePass()
	{
		SetPassName("Test");
		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Opaque);
		_sPso.SetName("Test");

		SetInputLayout(ERenderType::Static, D3D12::Input::StaticLayout);
		SetVS(ERenderType::Static, "Asset/Shader/Source/TestShader/TestVS.cso");

		SetPS("Asset/Shader/Source/TestShader/TestPS.cso");

		AddRead("Depth", AccessType::Depth_Write, LoadOp::Load, StoreOp::Store);
		AddWrite("Test", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}