#include "DeltaTime.h"

void DeltaTime::CalcDeltaTime()
{
	// 現在の時間を取得
	auto _currentTime = std::chrono::high_resolution_clock::now();

	// 経過時間を計算(ミリ秒単位)
	m_dt = std::chrono::duration_cast<std::chrono::milliseconds>(_currentTime - m_previousTime).count();

	// 秒単位に変換
	m_dt *= 0.001f;

	// 前回の時間を更新
	m_previousTime = _currentTime;
}
