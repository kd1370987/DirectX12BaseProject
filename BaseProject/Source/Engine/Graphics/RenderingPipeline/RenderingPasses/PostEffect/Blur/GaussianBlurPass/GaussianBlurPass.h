#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GaussianBlurPass
	//
	// 汎用のガウシアンブラー。入力と出力で解像度が違ってよいので、
	// 縮小(ダウンサンプリング)にも拡大にも使える。
	//
	// 旧版はブルームの縮小4段を for ループで登録していたが、
	// 反復はノードを並べて表現する方針なので、1ノード＝1段になる。
	// 4段かけたければこのノードを4つ置いて数珠つなぎにすること
	//======================================================================================
	class GaussianBlurPass : public Pass
	{
	public:
		~GaussianBlurPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// コードから組むとき用 : 段ごとに変える値をまとめて決める
		void Configure(const std::string& a_resourceName, float a_outputScale, float a_sigma, int a_tapRadius)
		{
			m_params.resourceName = a_resourceName;
			m_params.outputScale = a_outputScale;
			m_params.sigma = a_sigma;
			m_params.tapRadius = a_tapRadius;
			ApplyOutputScale();
		}

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		struct Params
		{
			// 出力リソース名 : 同じ名前だと縮小段どうしが同じリソースを取り合うので、
			// 段ごとに変えられるようにしてある
			std::string resourceName = "BlurResult";

			// 出力の解像度(画面に対する倍率)
			float outputScale = 0.5f;

			float sigma = 1.2f;		// ガウス分布の標準偏差(入力テクセル単位)
			int tapRadius = 2;		// 片側のタップ数
		};
		Params& RefParams() { return m_params; }

		// 出力の解像度スケールをスロットへ反映する。
		// スロットは SetupSlots で作られ、値の読み込みはその後なので、
		// 読み込み後と編集後に必ず通す
		void ApplyOutputScale();

	private:

		Params m_params = {};
	};
}
