#include "ToneMapPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void ToneMapPass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		// ここでLDRへ落ちる
		DeclareOutput("Result", "FinalColor", DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void ToneMapPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/ToneMap/ToneMapShader.cso", "ToneMapShader");
	}

	void ToneMapPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, m_cb);
		DispatchFullScreen(a_context);
	}



	void ToneMapPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("type", m_cb.type);
		a_arch.Field("exposure", m_cb.exposure);
		a_arch.Field("whitePoint", m_cb.whitePoint);
	}
}
