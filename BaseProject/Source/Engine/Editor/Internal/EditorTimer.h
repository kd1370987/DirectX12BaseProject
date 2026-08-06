#pragma once
namespace Engine::Editor
{
	/// <summary>
	/// すべて MS (ミリ秒) での計測を前提
	///
	/// 計測と集計は Profiler / CPUProfiler / GPUProfiler が行い、
	/// パネル側はここに入った値を読むだけにする
	/// </summary>
	struct Timer
	{
		// CPU時間
		std::chrono::high_resolution_clock::time_point startTime;		// 計測開始ポイント
		std::chrono::high_resolution_clock::time_point endTime;			// 計測終了ポイント

		double time = 0.0;					// 現在フレームの時間
		double accumulatedTime = 0.0;		// 累積時間

		//---------------------------------------------------------------------
		// 集計結果 : 表示側はここを読むだけ
		//---------------------------------------------------------------------
		double averageTime = 0.0;			// 平均時間 (平均レートごとに更新)
		double minTime = 0.0;				// リセット以降の最小時間
		double maxTime = 0.0;				// リセット以降の最大時間

		int sampleCount = 0;				// 平均計算用のサンプル数 (平均確定時に0へ戻る)
		int totalCount = 0;					// リセット以降の総計測回数

		bool isMeasuring = false;			// Start済みかどうか (Stopのみ呼ばれた場合の弾き用)
		bool hasSample = false;				// 一度でも計測できたか (min/maxの初期化判定)

		// GPU時間
	};
}
