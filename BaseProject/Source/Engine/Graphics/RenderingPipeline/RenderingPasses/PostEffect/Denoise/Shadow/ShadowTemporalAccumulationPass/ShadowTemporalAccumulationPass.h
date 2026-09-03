#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// ShadowTemporalAccumulationPass
	//
	// レイトレ影を前フレームの結果と混ぜて、時間方向にノイズをならす。
	//
	// 旧版は偶数/奇数フレーム用の2本で履歴を入れ替えていたが、
	// History 出力を Temporal にすることで1本で済ませている。
	// HistoryOut を History へ繋ぐと、前フレームが書いたほうが入ってくる
	//======================================================================================
	class ShadowTemporalAccumulationPass : public Pass
	{
	public:
		~ShadowTemporalAccumulationPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ送る調整値
		// ※ HLSL 側と並びを合わせること(GIのテンポラルと同じ形)
		struct ShadowTACB
		{
			float phiDepth;		// 深度の感度(履歴を捨てる判定)
			float phiNormal;	// 法線の感度
			float blendRate;	// 今フレームを混ぜる割合
		};
		ShadowTACB m_cb = { 1.0f, 32.0f, 0.1f };
	};
}
