#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GroundFogCompositePass
	//
	// メインカラーへ、GroundVolumetricFogPass が書いたフォグを重ねる。
	//   出力 = lerp(メインカラー, フォグの色, フォグの濃さ * intensity)
	//
	// トーンマップ前(HDR)に置くこと。
	// フォグはUVで引くので、フォグ側だけ縮小解像度で回しても合わせられる
	//======================================================================================
	class GroundFogCompositePass final : public Pass
	{
	public:
		~GroundFogCompositePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		// シェーダーへ送る設定
		// ※ HLSL 側(GroundFogCompositeCS の GroundFogCompositeData)と並びを合わせること
		struct CompositeCB
		{
			float intensity = 1.0f;		// フォグの濃さに掛ける倍率
			int   enable = 1;			// 0 なら重ねずにそのまま通す
			float pad0[2] = {};
		};

		using Params = CompositeCB;
		Params& RefParams() { return m_cb; }

	private:

		// ルートパラメータの番号 : シェーダー(GroundFogCompositeCS)の並びと合わせる
		static constexpr int kRootCompositeCB = 0;
		static constexpr int kRootInputSRV = 1;
		static constexpr int kRootOutputUAV = 2;

		CompositeCB m_cb = {};
	};
}
