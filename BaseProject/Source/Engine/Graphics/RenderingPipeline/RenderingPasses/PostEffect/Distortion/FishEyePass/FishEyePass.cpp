#include "FishEyePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void FishEyePass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		DeclareOutput("Result", "FishEyeColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void FishEyePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Distortion/FishEyeCS.cso", "FishEyeCS");
	}

	void FishEyePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, m_cb);
		DispatchFullScreen(a_context);
	}

	EPassEditResult FishEyePass::EditUpdate()
	{
		bool _isEdit = false;

		bool _isEnable = (m_cb.enable != 0);
		if (ImGui::Checkbox("Enable", &_isEnable)) { m_cb.enable = _isEnable ? 1 : 0; _isEdit = true; }

		_isEdit |= ImGui::DragFloat2("Center", &m_cb.center.x, 0.01f);

		// 正で樽型、負で糸巻き型
		_isEdit |= ImGui::DragFloat("Strength", &m_cb.strength, 0.01f, -2.0f, 2.0f);

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void FishEyePass::EditNode()
	{}

	void FishEyePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("centerX", m_cb.center.x);
		a_arch.Field("centerY", m_cb.center.y);
		a_arch.Field("strength", m_cb.strength);
		a_arch.Field("enable", m_cb.enable);
	}
}
