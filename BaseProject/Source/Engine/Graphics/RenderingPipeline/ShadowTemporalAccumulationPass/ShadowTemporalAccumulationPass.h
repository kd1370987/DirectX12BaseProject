#pragma once

#include "../RenderingPipeline.h"

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
	};
}
