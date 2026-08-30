#pragma once

#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// TAAPass
	//
	// 前フレームの結果と混ぜて、ちらつきを抑える。
	//
	// 旧版は偶数フレーム用と奇数フレーム用のパスを2本登録して履歴を入れ替えていたが、
	// こちらは History 出力を Temporal にすることで1本で済ませている。
	// 入力の History は「前フレームが書いたほう」を自動で指す
	//======================================================================================
	class TAAPass : public Pass
	{
	public:
		~TAAPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 履歴の入出力 : この2つを繋ぐことで前フレームの結果が入ってくる
		static constexpr const char* kHistoryInName = "History";
		static constexpr const char* kHistoryOutName = "HistoryOut";
	};
}
