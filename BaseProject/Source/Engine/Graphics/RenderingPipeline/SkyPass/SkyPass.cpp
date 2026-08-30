#include "SkyPass.h"

#include "../../RenderContext/RenderContext.h"
#include "../../GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void SkyPass::SetupSlots()
	{
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, kRootDepthSRV);

		// 色と速度へ書き足す。すでに描かれているぶんは残すので Load
		Slot& _color = DeclareOutput("Color", "AfterLighting", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, kRootColorUAV);
		_color.loadOp = ELoadOp::Load;

		Slot& _velocity = DeclareOutput("Velocity", "GBufferVelocity", DXGI_FORMAT_R16G16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, kRootVelocityUAV);
		_velocity.loadOp = ELoadOp::Load;
	}

	void SkyPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/Sky/SkyShader.cso", "SkyShader");
	}

	void SkyPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		auto* _pCtx = a_context.pRenderContext;
		auto* _pGE = a_context.pGraphicsEngine;

		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(a_context.pCmdList, kRootCameraCB, _pGE->GetCameraData());
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<SkyData>(a_context.pCmdList, kRootSkyCB, _pGE->GetSkyData());

		// スカイテクスチャはシーン側が差し替えるので、空のフレームは描かない
		const auto& _skyTexHandle = _pGE->GetSkyTexture();
		const auto* _pSkyTex = Resource::ResourceManager::Instance().Get(_skyTexHandle);
		if (!_pSkyTex) return;

		_pCtx->ComputeBindSRV(kRootSkyTexSRV, _pSkyTex->GetSRV());

		DispatchFullScreen(a_context);
	}

	EPassEditResult SkyPass::EditUpdate()
	{
		ImGui::TextDisabled("空の設定は SceneAmbientObject の持ち物です");
		return EPassEditResult::None;
	}

	void SkyPass::EditNode()
	{}

	void SkyPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
