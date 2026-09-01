#include "DoFPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void DoFPass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル(色, CoC) / 2=UAV
		// 同じ番号を指定した入力は、宣言した順にテーブルへ並ぶ
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);
		DeclareInput("CoC", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		DeclareOutput("Result", "DoFColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void DoFPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/DoF/DoFShader.cso", "DoFShader");
	}

	void DoFPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, m_cb);
		DispatchFullScreen(a_context);
	}

	EPassEditResult DoFPass::EditUpdate()
	{
		bool _isEdit = false;

		bool _isEnable = (m_cb.enable != 0);
		if (ImGui::Checkbox("Enable", &_isEnable)) { m_cb.enable = _isEnable ? 1 : 0; _isEdit = true; }

		_isEdit |= ImGui::DragFloat("FocusDistance", &m_cb.focusDistance, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("FocusRange", &m_cb.focusRange, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("NearRange", &m_cb.nearRange, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("FarRange", &m_cb.farRange, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("MaxBlurRadius", &m_cb.maxBlurRadius, 0.1f, 0.0f);

		ImGui::TextDisabled("CoCPass と同じ値にすること");

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void DoFPass::EditNode()
	{}

	void DoFPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("focusDistance", m_cb.focusDistance);
		a_arch.Field("focusRange", m_cb.focusRange);
		a_arch.Field("nearRange", m_cb.nearRange);
		a_arch.Field("farRange", m_cb.farRange);
		a_arch.Field("maxBlurRadius", m_cb.maxBlurRadius);
		a_arch.Field("enable", m_cb.enable);
	}
}
