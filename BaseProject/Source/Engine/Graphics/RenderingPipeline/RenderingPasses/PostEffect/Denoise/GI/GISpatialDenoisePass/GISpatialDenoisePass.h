#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GISpatialDenoisePass
	//
	// レイトレGIのノイズを、エッジを保ちながら空間方向にならす。ハーフ解像度で回る。
	//
	// 旧版は StepSize を倍にしながら for ループで複数回登録していたが、
	// 反復はノードを並べて表現する方針なので 1ノード＝1回になる。
	// 5回かけたければこのノードを5つ置き、出力を次の入力へ繋いでいくこと。
	// そのとき StepSize は 1, 2, 4, 8, 16 のように段ごとに変える
	//======================================================================================
	class GISpatialDenoisePass : public Pass
	{
	public:
		~GISpatialDenoisePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// コードから組むとき用 : 段ごとに変える値をまとめて決める
		void Configure(const std::string& a_resourceName, int a_stepSize)
		{
			m_params.resourceName = a_resourceName;
			m_params.cb.stepSize = a_stepSize;
			ApplyResourceName();
		}

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		// シェーダーへ送る調整値
		struct DenoiseCB
		{
			int   stepSize;		// タップの間隔。段ごとに倍にしていく
			float phiDepth;		// 深度の感度(小さいほどエッジを厳密に保護)
			float phiNormal;	// 法線の感度(大きいほど法線のずれに敏感)
			float phiColor;		// 輝度の感度(ノイズとディティールの境界制御)
		};

		struct Params
		{
			// 出力リソース名 : 段を複数置くときに取り合わないよう変えられるようにする
			std::string resourceName = "DenoisedGI";

			DenoiseCB cb = { 1, 1.0f, 32.0f, 4.0f };
		};
		Params& RefParams() { return m_params; }

		// スロットへ出力名を反映する
		void ApplyResourceName();

	private:

		Params m_params = {};
	};
}
