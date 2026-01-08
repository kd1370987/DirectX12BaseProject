#pragma once

#pragma comment(lib,"winmm.lib")

class FPSController
{
public:

	void SetMaxFPS(UINT a_fps);

	void BeginFrame();
	void EndFrame(bool a_isVsync);

	UINT GetNowFPS() { return m_nowFPS; }

private:

	void FPSMonitor();

	// FPS制御
	UINT m_nowFPS = 0;		// 現在のFPS値
	UINT m_maxFPS = 60;		// 最大FPS

	UINT m_frameCount = 0;		// フレームレート計測用
	std::chrono::steady_clock::time_point m_frameStart;
	std::chrono::steady_clock::time_point m_countFrameStart;
	std::chrono::duration<double> m_target{ 0 };
};
