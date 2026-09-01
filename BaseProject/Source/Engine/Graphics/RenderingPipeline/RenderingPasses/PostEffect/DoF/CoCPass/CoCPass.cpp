#include "CoCPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void CoCPass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=DoF調整値CB / 2=SRVテーブル / 3=UAV / 4=スカイCB
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);

		DeclareOutput("CoC", "CoC", DXGI_FORMAT_R16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 3);
	}

	void CoCPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/DoF/CoCShader.cso", "CoCShader");
	}

	void CoCPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		auto* _pCB = a_context.pRenderContext->BindCB();

		// カメラとスカイはエンジンの持ち物。パスの調整値ではないのでそのまま引く
		_pCB->BindAndAttachDataComputeRootCBV<CameraData>(a_context.pCmdList, 0, a_context.pGraphicsEngine->GetCameraData());
		_pCB->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, m_cb);
		_pCB->BindAndAttachDataComputeRootCBV<SkyData>(a_context.pCmdList, 4, a_context.pGraphicsEngine->GetSkyData());

		DispatchFullScreen(a_context);
	}

	EPassEditResult CoCPass::EditUpdate()
	{
		bool _isEdit = false;

		bool _isEnable = (m_cb.enable != 0);
		if (ImGui::Checkbox("Enable", &_isEnable)) { m_cb.enable = _isEnable ? 1 : 0; _isEdit = true; }

		_isEdit |= ImGui::DragFloat("FocusDistance", &m_cb.focusDistance, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("FocusRange", &m_cb.focusRange, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("NearRange", &m_cb.nearRange, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("FarRange", &m_cb.farRange, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("MaxBlurRadius", &m_cb.maxBlurRadius, 0.1f, 0.0f);

		// DoFPass 側にも同じ調整値がある。両方を合わせること
		ImGui::TextDisabled("DoFPass と同じ値にすること");

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void CoCPass::EditNode()
	{}

	void CoCPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("focusDistance", m_cb.focusDistance);
		a_arch.Field("focusRange", m_cb.focusRange);
		a_arch.Field("nearRange", m_cb.nearRange);
		a_arch.Field("farRange", m_cb.farRange);
		a_arch.Field("maxBlurRadius", m_cb.maxBlurRadius);
		a_arch.Field("enable", m_cb.enable);
	}
}
