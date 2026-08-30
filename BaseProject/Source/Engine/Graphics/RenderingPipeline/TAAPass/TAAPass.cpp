#include "TAAPass.h"

#include "../../RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void TAAPass::SetupSlots()
	{
		// ルートパラメータ : 0=SRVテーブル / 1=UAV
		// テーブルの並びはシェーダー側の t0..t4 と同じ順にすること
		DeclareInput("Color", EAccessType::SRV, EPassSlotType::Texture, true, 0);
		DeclareInput(kHistoryInName, EAccessType::SRV, EPassSlotType::Texture, true, 0);
		DeclareInput("Velocity", EAccessType::SRV, EPassSlotType::Texture, true, 0);
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, 0);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, 0);

		// フレーム間で入れ替わる履歴。
		// Temporal を立てると物理を2枚持ち、書く側と読む側が毎フレーム入れ替わる
		DeclareOutput(kHistoryOutName, "TAAHistory", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, true, 1);
	}

	void TAAPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/TAA/TAA.cso", "TAAShader");
	}

	void TAAPass::Update(const PassContext& a_context)
	{
		if (!a_context.pRenderContext) return;

		// 調整値を持たないので、バインドはすべてグラフ側で済んでいる
		DispatchFullScreen(a_context);
	}

	EPassEditResult TAAPass::EditUpdate()
	{
		ImGui::TextDisabled("History 出力を History 入力へ繋いでください");
		ImGui::TextDisabled("(Temporal なので前フレームのぶんが入ります)");
		return EPassEditResult::None;
	}

	void TAAPass::EditNode()
	{}

	void TAAPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
