#include "KawaseBlurPass.h"

#include "../../RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void KawaseBlurPass::SetupSlots()
	{
		// ルートパラメータ : 0=SRVテーブル(4段) / 1=UAV
		// 並びはシェーダーの t0..t3 と同じ。粗い順ではなく細かい順(1/2 -> 1/16)
		DeclareInput("Down0", EAccessType::SRV, EPassSlotType::Texture, true, 0);	// 1/2
		DeclareInput("Down1", EAccessType::SRV, EPassSlotType::Texture, true, 0);	// 1/4
		DeclareInput("Down2", EAccessType::SRV, EPassSlotType::Texture, true, 0);	// 1/8
		DeclareInput("Down3", EAccessType::SRV, EPassSlotType::Texture, true, 0);	// 1/16

		DeclareOutput("Result", "BloomColor", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, 1);
	}

	void KawaseBlurPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/Bloom/KawaseBlurShader.cso", "KawaseBlurShader");
	}

	void KawaseBlurPass::Update(const PassContext& a_context)
	{
		// 調整値を持たないので、バインドはすべてグラフ側で済んでいる
		DispatchFullScreen(a_context);
	}

	EPassEditResult KawaseBlurPass::EditUpdate()
	{
		ImGui::TextDisabled("縮小4段をまとめて1枚のブルームにします");
		ImGui::TextDisabled("Down0 が一番大きい段(1/2)です");
		return EPassEditResult::None;
	}

	void KawaseBlurPass::EditNode()
	{}

	void KawaseBlurPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
