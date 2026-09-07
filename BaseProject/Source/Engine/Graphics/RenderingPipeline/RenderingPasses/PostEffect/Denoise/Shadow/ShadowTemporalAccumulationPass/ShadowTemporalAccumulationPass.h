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


		void Archive(Engine::Persistence::Archive& a_arch) override;

		struct ShadowTACB
		{
			float phiDepth;		// 深度の感度(履歴を捨てる判定)
			float phiNormal;	// 法線の感度
			float blendRate;	// 今フレームを混ぜる割合
		};

		// 編集対象の値 : エディターはここだけを触る。
		// 実体は下の m_cb で、シェーダーへはそのまま送られる
		using Params = ShadowTACB;
		Params& RefParams() { return m_cb; }

	private:

		// シェーダーへ送る調整値
		// ※ HLSL 側と並びを合わせること(GIのテンポラルと同じ形)
		ShadowTACB m_cb = { 1.0f, 32.0f, 0.1f };
	};
}
