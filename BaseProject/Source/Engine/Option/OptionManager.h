#pragma once

// グラフィックスオプション
#include "GraphicsOptions/GIOptions.h"
#include "GraphicsOptions/WindowOption.h"

namespace Engine::Option
{
	class OptionManager
	{
	public:

		// シリアライズ・デシリアライズ
		void Serialize();
		void Deserialize();

		// エディター描画
		void DrawEdit();

		// ---- アクセサ ----
		// GI設定
		const GraphicsOptions::GIOption& GetGIOption() const { return m_giOptions; }
		GraphicsOptions::GIOption& RefGIOption() { return m_giOptions; }

		// ウィンドウ設定
		const GraphicsOptions::WindowOption& GetWindowOption() const { return m_windowOption; }
		GraphicsOptions::WindowOption& RefWindowOption() { return m_windowOption; }

	private:

		// グラフィックスオプション
		GraphicsOptions::GIOption m_giOptions = {};
		GraphicsOptions::WindowOption m_windowOption = {};

	// シングルトン
	private:
		OptionManager() = default;
		~OptionManager() = default;
	public:

		static OptionManager& GetInstance()
		{
			static OptionManager _instance;
			return _instance;
		}
	};
}