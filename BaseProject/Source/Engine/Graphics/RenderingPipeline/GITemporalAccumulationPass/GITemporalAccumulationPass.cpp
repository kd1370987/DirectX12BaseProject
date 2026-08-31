#include "GITemporalAccumulationPass.h"

#include "../../RenderContext/RenderContext.h"
#include "../../GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void GITemporalAccumulationPass::SetupSlots()
	{
		// ルートパラメータ : 0=カメラCB / 1=調整値CB / 2=SRVテーブル / 3=UAV
		DeclareInput("GI", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Velocity", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		// 履歴は前フレームの結果を読むピン。
		// 実行順の辺にならないので、自分の出力へそのまま繋いで回せる
		DeclareInput("History", EAccessType::SRV, EPassSlotType::Texture, true, 2, true);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		// 1つ前のフレームのGBuffer。前フレームを読むピンなので実行順の辺にならない
		DeclareInput("PrevDepth", EAccessType::SRV, EPassSlotType::Texture, true, 2, true);
		DeclareInput("PrevNormal", EAccessType::SRV, EPassSlotType::Texture, true, 2, true);

		// GIはハーフ解像度。フレーム間で入れ替わる履歴でもある
		Slot& _out = DeclareOutput("HistoryOut", "GITAHistory", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, true, 3);
		_out.scale = 0.5f;
	}

	void GITemporalAccumulationPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context,
			"Asset/Shader/Source/Lighting/Denoise/GI/TemporalAccumulationShader.cso",
			"TemporalAccumulationPass");
	}

	void GITemporalAccumulationPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		// カメラCB(b0) : シェーダー側でビュー空間を復元して履歴の棄却判定に使う
		a_context.pRenderContext->ComputeBindRootCBV(0, a_context.pGraphicsEngine->GetCameraData());
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, m_cb);

		const Slot* _pOut = FindOutputSlot(MakeSlotID("HistoryOut"));
		if (_pOut) DispatchForSlot(a_context, *_pOut);
	}

	EPassEditResult GITemporalAccumulationPass::EditUpdate()
	{
		bool _isEdit = false;

		_isEdit |= ImGui::DragFloat("PhiDepth", &m_cb.phiDepth, 0.01f, 0.0f);
		_isEdit |= ImGui::DragFloat("PhiNormal", &m_cb.phiNormal, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("BlendRate", &m_cb.blendRate, 0.01f, 0.0f, 1.0f);

		ImGui::TextDisabled("HistoryOut を History へ繋いでください");

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void GITemporalAccumulationPass::EditNode()
	{}

	void GITemporalAccumulationPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("phiDepth", m_cb.phiDepth);
		a_arch.Field("phiNormal", m_cb.phiNormal);
		a_arch.Field("blendRate", m_cb.blendRate);
	}
}
