#include "RadialBlurPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void RadialBlurPass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		// 全画素を書き潰すのでクリアは不要。
		// トーンマップ前なので、メインカラーと同じHDRで受ける
		DeclareOutput("Result", "RadialBlurColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);
	}

	void RadialBlurPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Blur/RadialBlurCS.cso", "RadialBlurCS");
	}

	void RadialBlurPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;

		// ヒープ・ルートシグネチャ・PSO・SRV/UAV はグラフが張り終えている
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 0, m_cb);
		DispatchFullScreen(a_context);
	}

	EPassEditResult RadialBlurPass::EditUpdate()
	{
		bool _isEdit = false;

		bool _isEnable = (m_cb.enable != 0);
		if (ImGui::Checkbox("Enable", &_isEnable)) { m_cb.enable = _isEnable ? 1 : 0; _isEdit = true; }

		_isEdit |= ImGui::DragFloat2("BlurCenter", &m_cb.blurCenter.x, 0.01f);
		_isEdit |= ImGui::DragFloat("Strength", &m_cb.strength, 0.001f, 0.0f, 1.0f);
		_isEdit |= ImGui::DragInt("SampleCount", &m_cb.sampleCount, 1, 1, 64);
		_isEdit |= ImGui::DragFloat("Radius", &m_cb.radius, 0.01f, 0.0f, 2.0f);
		_isEdit |= ImGui::DragFloat("Falloff", &m_cb.falloff, 0.01f, 0.0f, 8.0f);

		// 値が変わるだけなのでグラフは組み直さない
		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void RadialBlurPass::EditNode()
	{}

	void RadialBlurPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("blurCenterX", m_cb.blurCenter.x);
		a_arch.Field("blurCenterY", m_cb.blurCenter.y);
		a_arch.Field("strength", m_cb.strength);
		a_arch.Field("sampleCount", m_cb.sampleCount);
		a_arch.Field("radius", m_cb.radius);
		a_arch.Field("falloff", m_cb.falloff);
		a_arch.Field("enable", m_cb.enable);
	}
}
