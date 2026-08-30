#include "ShadowSpatialDenoisePass.h"

#include "../../RenderContext/RenderContext.h"
#include "../../GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void ShadowSpatialDenoisePass::SetupSlots()
	{
		// ルートパラメータ : 0=調整値CB / 1=SRVテーブル / 2=UAV
		DeclareInput("Shadow", EAccessType::SRV, EPassSlotType::Texture, true, 1);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 1);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 1);

		// GBufferと同じフル解像度
		DeclareOutput("Result", m_resourceName, DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::UAV, EPassSlotType::Texture, false, 2);

		ApplyResourceName();
	}

	void ShadowSpatialDenoisePass::ApplyResourceName()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_resourceName;
	}

	void ShadowSpatialDenoisePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/Denoise/Shadow/ShadowSpatialDenoiseShader.cso", "ShadowSpatialDenoisePass");
	}

	void ShadowSpatialDenoisePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		// このシェーダーはルート直置きのCBを使う
		a_context.pRenderContext->ComputeBindRootCBV(0, m_cb);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}

	EPassEditResult ShadowSpatialDenoisePass::EditUpdate()
	{
		bool _isParam = false;
		bool _isStructure = false;

		char _nameBuf[128] = {};
		std::snprintf(_nameBuf, sizeof(_nameBuf), "%s", m_resourceName.c_str());
		if (ImGui::InputText("ResourceName", _nameBuf, sizeof(_nameBuf)))
		{
			m_resourceName = _nameBuf;
			_isStructure = true;
		}

		// 段ごとに 1, 2, 4, 8... と変える
		_isParam |= ImGui::DragInt("StepSize", &m_cb.stepSize, 1, 1, 64);
		_isParam |= ImGui::DragFloat("PhiDepth", &m_cb.phiDepth, 0.01f, 0.0f);
		_isParam |= ImGui::DragFloat("PhiNormal", &m_cb.phiNormal, 0.1f, 0.0f);
		_isParam |= ImGui::DragFloat("PhiColor", &m_cb.phiColor, 0.01f, 0.0f);

		if (_isStructure)
		{
			ApplyResourceName();
			return EPassEditResult::Structure;
		}
		return _isParam ? EPassEditResult::Param : EPassEditResult::None;
	}

	void ShadowSpatialDenoisePass::EditNode()
	{}

	void ShadowSpatialDenoisePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_resourceName);
		a_arch.Field("stepSize", m_cb.stepSize);
		a_arch.Field("phiDepth", m_cb.phiDepth);
		a_arch.Field("phiNormal", m_cb.phiNormal);
		a_arch.Field("phiColor", m_cb.phiColor);

		if (a_arch.IsLoading()) ApplyResourceName();
	}
}
