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

		const std::string& GetName() const override
		{
			static const std::string NAME = "RenderingOption";
			return NAME;
		}

		// カテゴリー
		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		// エディター
		void DrawEdit(const ECS::EngineServices& a_services) override;

		// アーカイブ
		void Archive(Persistence::Archive& a_archive) override;
	};
}