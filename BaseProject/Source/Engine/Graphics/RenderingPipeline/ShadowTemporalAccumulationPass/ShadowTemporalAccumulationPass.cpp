#include "ShadowTemporalAccumulationPass.h"

#include "../../RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void ShadowTemporalAccumulationPass::SetupSlots()
	{
		// ルートパラメータ : 2=SRVテーブル / 3=UAV
		// 並びはシェーダーの t0.. と同じ順にすること
		DeclareInput("Shadow", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Velocity", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("History", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("PrevDepth", EAccessType::SRV, EPassSlotType::Texture, true, 2);
		DeclareInput("PrevNormal", EAccessType::SRV, EPassSlotType::Texture, true, 2);

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
		// 調整値を持たないので、バインドはすべてグラフ側で済んでいる
		DispatchFullScreen(a_context);
	}

	EPassEditResult ShadowTemporalAccumulationPass::EditUpdate()
	{
		ImGui::TextDisabled("HistoryOut を History へ繋いでください");
		ImGui::TextDisabled("(Temporal なので前フレームのぶんが入ります)");
		return EPassEditResult::None;
	}

	void ShadowTemporalAccumulationPass::EditNode()
	{}

	void ShadowTemporalAccumulationPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
