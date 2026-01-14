#include "FPSController.h"

void FPSController::SetMaxFPS(UINT a_fps)
{
	m_maxFPS = a_fps;
	m_target = std::chrono::duration<double>(1.0f / a_fps);
}

void FPSController::BeginFrame()
{
	m_frameStart = std::chrono::steady_clock::now();
}

void FPSController::EndFrame(bool a_isVsync)
{
	// 垂直同期がない場合のみFPSを制御する
	if (!a_isVsync)
	{
		// ターゲット時間までスリープ
		while (true)
		{
			auto _elapsed = std::chrono::steady_clock::now() - m_frameStart;
			if (_elapsed >= m_target) break;
			if (m_target - _elapsed > std::chrono::milliseconds(2))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			else
			{
				_mm_pause();
			}
		}
	}

	// 現在のFPSを測定
	FPSMonitor();
}

void FPSController::FPSMonitor()
{
	constexpr auto _refresh = std::chrono::milliseconds(500);

	m_frameCount++;

	auto _now = std::chrono::steady_clock::now();
	auto _elapsed = _now - m_countFrameStart;

	if (_elapsed >= _refresh)
	{
		double _seconds = std::chrono::duration<double>(_elapsed).count();

		m_nowFPS = m_frameCount / _seconds;

		m_countFrameStart = _now;
		m_frameCount = 0;
	}
}

