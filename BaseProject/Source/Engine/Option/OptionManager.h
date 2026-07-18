#pragma once

// グラフィックスオプション

#include "IOption.h"
#include "GraphicsOptions/GIOptions.h"
#include "GraphicsOptions/WindowOption.h"
#include "GraphicsOptions/RenderingOption.h"
#include "GraphicsOptions/LightingOption.h"

namespace Engine::Option
{
	class OptionManager
	{
	public:

		// 初期化
		void Init();

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

		// レンダリング設定
		const GraphicsOptions::RenderingOption& GetRenderingOption() const { return m_renderingOption; }
		GraphicsOptions::RenderingOption& RefRenderingOption() { return m_renderingOption; }

		// ライティング設定(シェーダーへ送る調整値)
		const GraphicsOptions::LightingOption& GetLightingOption() const { return m_lightingOption; }
		GraphicsOptions::LightingOption& RefLightingOption() { return m_lightingOption; }

	private:

		void Archive(Persistence::Archive& a_ar);

	private:

		// グラフィックスオプション
		GraphicsOptions::GIOption m_giOptions = {};
		GraphicsOptions::WindowOption m_windowOption = {};
		GraphicsOptions::RenderingOption m_renderingOption = {};
		GraphicsOptions::LightingOption m_lightingOption = {};

		// ループ処理用
		std::vector<IOption*> m_pOptionList;

	// シングルトン
	private:
		OptionManager();
		~OptionManager() = default;
	public:

		static OptionManager& GetInstance()
		{
			static OptionManager _instance;
			return _instance;
		}
	};
}