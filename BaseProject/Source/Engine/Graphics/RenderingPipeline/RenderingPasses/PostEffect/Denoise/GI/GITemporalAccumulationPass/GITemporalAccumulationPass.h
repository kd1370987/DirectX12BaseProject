#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GITemporalAccumulationPass
	//
	// レイトレGIを前フレームの結果と混ぜて、時間方向にノイズをならす。ハーフ解像度。
	//
	// 旧版は偶数/奇数フレーム用の2本。こちらは History 出力を Temporal にして1本
	//======================================================================================
	class GITemporalAccumulationPass : public Pass
	{
	public:
		~GITemporalAccumulationPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		struct GITACB
		{
			float phiDepth;		// 深度の感度(履歴を捨てる判定)
			float phiNormal;	// 法線の感度
			float blendRate;	// 今フレームを混ぜる割合
		};

		// 編集対象の値 : エディターはここだけを触る。
		// 実体は下の m_cb で、シェーダーへはそのまま送られる
		using Params = GITACB;
		Params& RefParams() { return m_cb; }

	private:

		// シェーダーへ送る調整値
		GITACB m_cb = { 1.0f, 32.0f, 0.1f };
	};
}
