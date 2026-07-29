#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// GIのスペースデノイズの設定
	struct RenderingOption : IOption
	{
	
		bool isZPre = true;

		// TAA用のカメラジッター(サブピクセル揺らし)を有効にするか。
		// OFFにするとジッターが止まり、TAAはブレンドのみ(空間的なAA効果は無くなる)になる。デバッグ用。
		bool useJitter = true;

		Engine::GUID defaultShadingModelTable = {};

		const std::string& GetName() override
		{
			static const std::string _name = "RenderingOption";
			return _name;
		}

		// カテゴリー
		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		// エディター
		void DrawEdit() override;

		// アーカイブ
		void Archive(Persistence::Archive& a_archive) override;
	};
}