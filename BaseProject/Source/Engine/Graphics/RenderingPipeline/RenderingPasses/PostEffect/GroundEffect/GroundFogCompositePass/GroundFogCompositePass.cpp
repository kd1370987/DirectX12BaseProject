#include "GroundFogCompositePass.h"

#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void GroundFogCompositePass::SetupSlots()
	{
		// 同じ番号を指定した入力は、宣言した順にルート定数へ並ぶ
		// メインカラー(トーンマップ前)
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, kRootInputSRV);
		// フォグ(GroundVolumetricFogPass の出力)
		DeclareInput("Fog", EAccessType::SRV, EPassSlotType::Texture, true, kRootInputSRV);

		DeclareOutput("Result", "GroundFogCompositeColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, kRootOutputUAV);
	}

	void GroundFogCompositePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/GroundEffect/GroundFogCompositeCS.cso", "GroundFogCompositeCS");
	}

	void GroundFogCompositePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, kRootCompositeCB, m_cb);
		DispatchFullScreen(a_context);
	}



	void GroundFogCompositePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("intensity", m_cb.intensity);
		a_arch.Field("enable", m_cb.enable);
	}
}
