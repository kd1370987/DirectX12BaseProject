#pragma once
namespace Engine::Editor
{
	struct Timer;

	/// <summary>
	/// CPUの実行時間、メモリ使用率などを計るためのクラス
	/// ms 基準
	/// </summary>
	class CPUProfiler
	{
	public:

		// CPU時間計測
		void Start(Timer& a_timer);
		void Stop(Timer& a_timer);

		// 平均を確定させる : Profilerが平均レートごとに呼ぶ
		void FixAverage(Timer& a_timer);

		// 計測結果を初期化する
		void Reset(Timer& a_timer);
	};
}
