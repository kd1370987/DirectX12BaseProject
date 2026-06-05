#pragma once

// グラフィックスオプション
#include "GraphicsOptions/GIOptions.h"

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
		const GraphicsOptions::GIOption& GetGIOption() const { return m_giOptions; }
		GraphicsOptions::GIOption& RefGIOption() { return m_giOptions; }

	private:

		// グラフィックスオプション
		GraphicsOptions::GIOption m_giOptions = {};

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