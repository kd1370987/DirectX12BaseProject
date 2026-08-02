#pragma once

#include "../IOption.h"

#include "../../Editor/Helper/EditorHelper.h"

namespace Engine::Option::GraphicsOptions
{
	// GIのスペースデノイズの設定
	struct WindowOption : IOption
	{
		// ウィンドウタイトル
		std::string windowTitle = "Window";			// 名前
		bool isTitleFPS = false;					// FPSをタイトルに表示するか

		// ウィンドウサイズ
		int windowWidth = 0;
		int windowHeight = 0;

		// ウィンドウモード
		EWindowMode windowMode = EWindowMode::Windowed;
		
		// 垂直同期
		bool isVsync = false;

		// 最大フレームレート
		int targetFrameRate = 0;

		const std::string& GetName() override
		{
			static const std::string _name = "WindowOption";
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