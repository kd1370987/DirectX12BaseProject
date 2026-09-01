#include "BloomExtractPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void BloomExtractPass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		DeclareOutput("Result", "BloomExtract", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void BloomExtractPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Bloom/BloomExtractShader.cso", "BloomExtractShader");
	}

	void BloomExtractPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, m_cb);
		DispatchFullScreen(a_context);
	}

	EPassEditResult BloomExtractPass::EditUpdate()
	{
		bool _isEdit = false;

		bool _isEnable = (m_cb.enable != 0);
		if (ImGui::Checkbox("Enable", &_isEnable)) { m_cb.enable = _isEnable ? 1 : 0; _isEdit = true; }

		_isEdit |= ImGui::DragFloat("Threshold", &m_cb.threshold, 0.01f, 0.0f);
		_isEdit |= ImGui::DragFloat("SoftKnee", &m_cb.softKnee, 0.01f, 0.0f, 1.0f);

		// 強さは合成側で効く。抽出側は同じCBを使うので並びを合わせて持っている
		_isEdit |= ImGui::DragFloat("Intensity", &m_cb.intensity, 0.01f, 0.0f);

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void BloomExtractPass::EditNode()
	{}

	void BloomExtractPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("threshold", m_cb.threshold);
		a_arch.Field("softKnee", m_cb.softKnee);
		a_arch.Field("intensity", m_cb.intensity);
		a_arch.Field("enable", m_cb.enable);
	}
}
