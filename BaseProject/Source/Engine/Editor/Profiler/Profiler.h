#pragma once

#include "../Internal/EditorTimer.h"

namespace Engine::Editor
{

	class CPUProfiler;
	class GPUProfiler;

	/// <summary>
	/// 表示用の計測結果
	/// パネルはこの配列を上から並べるだけでよい
	/// </summary>
	struct TimerResult
	{
		std::string name;			// 計測名
		Timer		timer;			// 計測結果のスナップショット
	};

	/// <summary>
	/// エンジン全体の時間計測と、実行時間の監視
	///
	/// 計測・集計はすべてここが持ち、パネルは GetResults() を読むだけにする
	/// </summary>
	class Profiler
	{
	public:

		// CPUProfiler / GPUProfiler は前方宣言なので、
		// unique_ptr の破棄をこのヘッダ側で組み立てられない。
		// 実体が見えている Profiler.cpp 側でデストラクタを作る
		Profiler();
		~Profiler();

		// プロファイラ初期化
		void Init();

		// プロファイラ更新
		void BeginFrame();
		void EndFrame();

		// アクセサ
		void SetAvelageRate(int a_rate) { m_avelageRate = (a_rate > 0) ? a_rate : 1; }
		int GetAvelageRate() const { return m_avelageRate; }

		//-------------------------------------------------------------------------------------
		// 時間計測
		//-------------------------------------------------------------------------------------
		// 各名前ごとのタイマーの管理
		void StartTimer(const std::string& a_name);
		void StopTimer(const std::string& a_name);

		// 計測結果を全部初期化する
		void ResetAll();

		//-------------------------------------------------------------------------------------
		// 表示用
		//-------------------------------------------------------------------------------------
		/// <summary>
		/// 平均時間の降順に並べ替え済みの計測結果を取得する
		/// 中身は EndFrame の時点のスナップショットなので、描画中に増減しない
		/// </summary>
		const std::vector<TimerResult>& GetResults() const { return m_results; }

	private:

		// プロファイラ
		std::unique_ptr<CPUProfiler> m_upCPUProfiler = nullptr;
		std::unique_ptr<GPUProfiler> m_upGPUProfiler = nullptr;

		// 実行時間計測構造体
		std::unordered_map<std::string, Timer> m_timers = {};

		// 表示用に並べ替えた結果 : EndFrameで作り直す
		std::vector<TimerResult> m_results = {};

		// 平均を求める際の分割レート
		int m_avelageRate = 60;
		int m_frameCount = 0;
	};
}
