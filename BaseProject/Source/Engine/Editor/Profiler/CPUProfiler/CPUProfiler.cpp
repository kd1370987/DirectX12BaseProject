#include "CPUProfiler.h"

#include "../../Internal/EditorTimer.h"

namespace Engine::Editor
{
	//======================================================================================
	// 計測開始
	//======================================================================================
	void CPUProfiler::Start(Timer& a_timer)
	{
		a_timer.startTime = std::chrono::high_resolution_clock::now();
		a_timer.isMeasuring = true;
	}

	//======================================================================================
	// 計測終了
	//======================================================================================
	void CPUProfiler::Stop(Timer& a_timer)
	{
		// Startを通っていない場合は、既定値の開始時刻との差でとんでもない値が出るので弾く
		if (!a_timer.isMeasuring) return;
		a_timer.isMeasuring = false;

		a_timer.endTime = std::chrono::high_resolution_clock::now();

		// 実行時間計算
		a_timer.time = std::chrono::duration<double, std::milli>(a_timer.endTime - a_timer.startTime).count();
		a_timer.accumulatedTime += a_timer.time;		// 平均値用に累積

		// 最小・最大
		// 初回は比較対象がないのでそのまま入れる
		if (!a_timer.hasSample)
		{
			a_timer.hasSample = true;
			a_timer.minTime = a_timer.time;
			a_timer.maxTime = a_timer.time;
			a_timer.averageTime = a_timer.time;		// 平均が確定するまでの暫定値
		}
		else
		{
			a_timer.minTime = std::min(a_timer.minTime, a_timer.time);
			a_timer.maxTime = std::max(a_timer.maxTime, a_timer.time);
		}

		++a_timer.sampleCount;
		++a_timer.totalCount;
	}

	//======================================================================================
	// 平均の確定
	// 累積をサンプル数で割って平均に反映し、次の区間のために累積を空にする
	//======================================================================================
	void CPUProfiler::FixAverage(Timer& a_timer)
	{
		if (a_timer.sampleCount <= 0) return;

		a_timer.averageTime = a_timer.accumulatedTime / static_cast<double>(a_timer.sampleCount);

		a_timer.accumulatedTime = 0.0;
		a_timer.sampleCount = 0;
	}

	//======================================================================================
	// リセット
	//======================================================================================
	void CPUProfiler::Reset(Timer& a_timer)
	{
		a_timer.time = 0.0;
		a_timer.accumulatedTime = 0.0;

		a_timer.averageTime = 0.0;
		a_timer.minTime = 0.0;
		a_timer.maxTime = 0.0;

		a_timer.sampleCount = 0;
		a_timer.totalCount = 0;

		a_timer.isMeasuring = false;
		a_timer.hasSample = false;
	}
}
