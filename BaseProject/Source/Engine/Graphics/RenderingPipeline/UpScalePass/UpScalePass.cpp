#include "UpScalePass.h"

#include "../../RenderContext/RenderContext.h"
#include "../../GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void UpScalePass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=調整値CB / 2〜4=SRV(個別) / 5=UAV
		// このシェーダーはテーブルではなく個別のルートパラメータへ張る
		DeclareInput("GI", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 3);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 4);

		// 出力はフル解像度
		DeclareOutput("Result", "UpScaledGI", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 5);
	}

	void UpScalePass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/Lighting/UpScale/UpScaleCS.cso", "FullRaytracingUpScaleShader");
	}

	void UpScalePass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		a_context.pRenderContext->ComputeBindRootCBV(0, a_context.pGraphicsEngine->GetCameraData());
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, m_cb);

		DispatchFullScreen(a_context);
	}

	EPassEditResult UpScalePass::EditUpdate()
	{
		bool _isEdit = false;

		_isEdit |= ImGui::DragFloat("PhiDepth", &m_cb.phiDepth, 0.01f, 0.0f);
		_isEdit |= ImGui::DragFloat("PhiNormal", &m_cb.phiNormal, 0.1f, 0.0f);

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void UpScalePass::EditNode()
	{}

	void UpScalePass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("phiDepth", m_cb.phiDepth);
		a_arch.Field("phiNormal", m_cb.phiNormal);
	}
}
