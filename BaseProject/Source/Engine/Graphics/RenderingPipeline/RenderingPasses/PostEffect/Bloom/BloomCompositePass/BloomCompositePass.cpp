#include "BloomCompositePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void BloomCompositePass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル(色, ブルーム) / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);
		DeclareInput("Bloom", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		DeclareOutput("Result", "BloomCompositeColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void BloomCompositePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Bloom/BloomCompositeShader.cso", "BloomCompositeShader");
	}

	void BloomCompositePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, m_cb);
		DispatchFullScreen(a_context);
	}



	void BloomCompositePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("threshold", m_cb.threshold);
		a_arch.Field("softKnee", m_cb.softKnee);
		a_arch.Field("intensity", m_cb.intensity);
		a_arch.Field("enable", m_cb.enable);
	}
}
