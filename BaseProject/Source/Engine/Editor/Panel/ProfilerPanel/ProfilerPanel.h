#pragma once

#include "../IPanel.h"

namespace Engine::Editor
{
	class Profiler;

	/// <summary>
	/// エンジンの計測結果を表示するパネル
	/// FPS/メモリなどの全体統計と、Profilerが積んだ関数ごとの計測結果を出す
	///
	/// 計測はProfilerの担当なので、ここは受け取った結果を並べるだけにする
	/// </summary>
	class ProfilerPanel : public IPanel
	{
	public:
		~ProfilerPanel() override = default;

		const char* GetName() const override { return "ProfilerPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;

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

		// 関数ごとの計測結果(Profilerが並べ替え済み)
		void DrawTimerTable(Profiler* a_pProfiler);
	};
}
