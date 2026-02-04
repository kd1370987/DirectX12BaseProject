#include "DeltaTime.h"

void DeltaTime::CalcDeltaTime()
{
	if(m_isFirstCalc)
	{
		// 初回計測時は前回時間を現在時間で初期化して終了
		m_previousTime = std::chrono::high_resolution_clock::now();
		m_isFirstCalc = false;
		m_dt = 0.0f;
		return;
	}

	// 現在の時間を取得
	auto _currentTime = std::chrono::high_resolution_clock::now();

	// 経過時間を計算(ミリ秒単位)
	m_dt = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(_currentTime - m_previousTime).count());

	// 秒単位に変換
	m_dt *= 0.001f;

	// 前回の時間を更新
	m_previousTime = _currentTime;

	// 最大値を設定して極端に大きな値にならないようにする
	m_dt = std::min(m_dt, 1.0f / 30.0f); 
}
