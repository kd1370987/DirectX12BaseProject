#include "ShadowTemporalAccumulationPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics::Pipeline
{
	void ShadowTemporalAccumulationPass::SetupSlots()
	{
		// ルートパラメータ : 2=SRVテーブル / 3=UAV
		// 並びはシェーダーの t0.. と同じ順にすること
		DeclareInput("Shadow", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Velocity", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		// 履歴は前フレームの結果を読むピン。
		// 実行順の辺にならないので、自分の出力へそのまま繋いで回せる
		DeclareInput("History", EAccessType::SRV, EPassSlotType::Texture, true, 2, true);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		// 1つ前のフレームのGBuffer。前フレームを読むピンなので実行順の辺にならない
		DeclareInput("PrevDepth", EAccessType::SRV, EPassSlotType::Texture, true, 2, true);
		DeclareInput("PrevNormal", EAccessType::SRV, EPassSlotType::Texture, true, 2, true);

		// フレーム間で入れ替わる履歴
		DeclareOutput("HistoryOut", "ShadowTAHistory", DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::UAV, EPassSlotType::Texture, true, 3);
	}

	void ShadowTemporalAccumulationPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context,
			"Asset/Shader/Source/Lighting/Denoise/Shadow/ShadowTemporalAccumullationShader.cso",
			"ShadowTemporalAccumullationShader");
	}

	void ShadowTemporalAccumulationPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext || !a_context.pCmdList) return;
		if (!a_context.pGraphicsEngine) return;

		// カメラCB(b0) : シェーダー側でビュー空間を復元して履歴の棄却判定に使う
		a_context.pRenderContext->ComputeBindRootCBV(0, a_context.pGraphicsEngine->GetCameraData());

		// 調整値CB(b1)。ここを張り忘れると、前のパスが残した中身で再投影の判定が回る
		a_context.pRenderContext->BindCB()->BindAndAttachDataComputeRootCBV(a_context.pCmdList, 1, m_cb);

		DispatchFullScreen(a_context);
	}

	EPassEditResult ShadowTemporalAccumulationPass::EditUpdate()
	{
		ImGui::TextDisabled("HistoryOut を History へ繋いでください");
		ImGui::TextDisabled("(Temporal なので前フレームのぶんが入ります)");

		bool _isEdit = false;
		_isEdit |= ImGui::DragFloat("PhiDepth", &m_cb.phiDepth, 0.01f, 0.0f);
		_isEdit |= ImGui::DragFloat("PhiNormal", &m_cb.phiNormal, 0.1f, 0.0f);
		_isEdit |= ImGui::DragFloat("BlendRate", &m_cb.blendRate, 0.01f, 0.0f, 1.0f);

		return _isEdit ? EPassEditResult::Param : EPassEditResult::None;
	}

	void ShadowTemporalAccumulationPass::EditNode()
	{}

	void ShadowTemporalAccumulationPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		a_arch.Field("phiDepth", m_cb.phiDepth);
		a_arch.Field("phiNormal", m_cb.phiNormal);
		a_arch.Field("blendRate", m_cb.blendRate);
	}
}
