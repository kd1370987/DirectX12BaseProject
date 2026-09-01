#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

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

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

		// コードから組むとき用 : 段ごとに変える値をまとめて決める
		void Configure(const std::string& a_resourceName, float a_outputScale, float a_sigma, int a_tapRadius)
		{
			m_resourceName = a_resourceName;
			m_outputScale = a_outputScale;
			m_sigma = a_sigma;
			m_tapRadius = a_tapRadius;
			ApplyOutputScale();
		}

	private:

		// 出力の解像度スケールをスロットへ反映する。
		// スロットは SetupSlots で作られ、値の読み込みはその後なので、
		// 読み込み後と編集後に必ず通す
		void ApplyOutputScale();

		// 出力リソース名 : 同じ名前だと縮小段どうしが同じリソースを取り合うので、
		// 段ごとに変えられるようにしてある
		std::string m_resourceName = "BlurResult";

		// 出力の解像度(画面に対する倍率)
		float m_outputScale = 0.5f;

		float m_sigma = 1.2f;		// ガウス分布の標準偏差(入力テクセル単位)
		int m_tapRadius = 2;		// 片側のタップ数
	};
}
