#pragma once

#include "../ImGui/Watch/Watch.h"

namespace Engine::Editor
{
	class WatchView
	{
	public:

		void Init();

		// 描画
		void Draw();

		// 任意の時間を図りたいとき
		void StartWatch(const std::string& a_name);
		void EndWatch(const std::string& a_name);

	private:
		// メモリ使用率(Ram)
		void DrawMemoryUsage();

		// VRAM使用率
		void DrawVRAMUsage();

		// CPU時間 : GPU時間
		void DrawCoreTimings();

		// FPS & デルタタイム
		void DrawFPSAndDeltaTime();

		// DrawCall数 : 総プリミティブ数 : アイテム
		void DrawRenderStats();

		// ディスクリプタヒープ使用率

		void DrawDescriptorHeapUsage();
	private:

		// ウォッチ用
		std::unordered_map<std::string, UINT> m_watchIndexMap = {};
		std::vector<Watch> m_watchVec = {};		// 実体の配列
		std::vector<Watch*> m_cache = {};		// ソートされた配列

	};
}