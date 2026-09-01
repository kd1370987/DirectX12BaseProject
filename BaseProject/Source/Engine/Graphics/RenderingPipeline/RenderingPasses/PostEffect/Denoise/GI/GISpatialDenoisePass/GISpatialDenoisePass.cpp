#include "GISpatialDenoisePass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void GISpatialDenoisePass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=調整値CB / 2=SRVテーブル / 3=UAV
		DeclareInput("GI", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 2);

		// GIはハーフ解像度。GBuffer(フル)とは倍率が違う
		DeclareOutput("Result", m_resourceName, DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 3);

		ApplyResourceName();
	}

	void GISpatialDenoisePass::ApplyResourceName()
	{
		Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (!_pOut) return;

		_pOut->name = m_resourceName;

		// GIはハーフ解像度で回る
		_pOut->width = 0;
		_pOut->height = 0;
		_pOut->scale = 0.5f;
	}

	void GISpatialDenoisePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/Denoise/GI/GISpatialDenoiseShader.cso", "GISpatialDenoisePass");
	}

	void GISpatialDenoisePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		// カメラCB(b0) : シェーダー側でワールド座標を復元してエッジ判定に使う
		a_context.pRenderContext->ComputeBindRootCBV(0, a_context.pGraphicsEngine->GetCameraData());
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, m_cb);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("Result"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}

	EPassEditResult GISpatialDenoisePass::EditUpdate()
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

	void GISpatialDenoisePass::EditNode()
	{}

	void GISpatialDenoisePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.StringField("resourceName", m_resourceName);
		a_arch.Field("stepSize", m_cb.stepSize);
		a_arch.Field("phiDepth", m_cb.phiDepth);
		a_arch.Field("phiNormal", m_cb.phiNormal);
		a_arch.Field("phiColor", m_cb.phiColor);

		if (a_arch.IsLoading()) ApplyResourceName();
	}
}
