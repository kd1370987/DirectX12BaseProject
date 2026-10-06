#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// ShadowMapPass
	//
	// 主光源から見た不透明モデルの深度(シャドウマップ)を描く。
	// 影マスクにするのは後ろの ShadowMapMaskPass。
	//
	// カスケードごとに 2x2 のタイルへ分けた1枚のアトラスへ描く。
	//   カスケード i のタイル : 左上から (i % 2, i / 2)
	// カスケードの行列は LightManager が毎フレーム組んだものを使う(読む側と同じものにするため)。
	//
	// 影の求め方がレイトレのフレームは何も描かない(深度はグラフがクリアしたまま)。
	// 描画アイテムはどのフレームも流れてくるので、パイプラインに置いたままでよい
	//======================================================================================
	class ShadowMapPass : public Pass
	{
	public:
		~ShadowMapPass() override = default;

		// シェーディングモデル表はこの名前で引く(表示名とは別)
		const char* GetShadingPassName() const override { return "ShadowMap"; }

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// アトラスの1辺あたりのタイル数 : カスケードは最大 2x2 = 4 枚
		static constexpr uint32_t ATLAS_TILES = 2;

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		struct Params
		{
			// アトラス全体の1辺(ピクセル)。タイル1枚はこの半分になる。
			// 変えるとテクスチャを作り直すので、グラフごと組み直しが要る
			uint32_t resolution = 4096;
		};
		Params& RefParams() { return m_params; }

		// 解像度をスロットへ反映する。
		// スロットは SetupSlots で作られ、値の読み込みはその後なので、
		// 読み込み後と編集後に必ず通す
		void ApplyResolution();

	private:

		Params m_params = {};
	};
}
