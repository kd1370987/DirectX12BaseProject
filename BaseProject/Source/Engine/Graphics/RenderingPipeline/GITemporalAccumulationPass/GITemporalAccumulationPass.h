#pragma once

#include "../RenderingPipeline.h"

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

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ送る調整値
		struct GITACB
		{
			float phiDepth;		// 深度の感度(履歴を捨てる判定)
			float phiNormal;	// 法線の感度
			float blendRate;	// 今フレームを混ぜる割合
		};
		GITACB m_cb = { 1.0f, 32.0f, 0.1f };
	};
}
