#pragma once


namespace Engine::Time
{
	class FPSController
	{
	public:
		/// <summary>
		/// 最大フレームレート設定
		/// </summary>
		void SetMaxFPS(UINT a_fps);

		/// <summary>
		/// フレームの初めに呼び出し : 記録開始
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// フレームの最後に呼び出し : スリープ処理
		/// </summary>
		/// <param name="a_isVsync">垂直同期かどうか</param>
		void EndFrame(bool a_isVsync);

		/// <summary>
		/// 現在のフレームレートを取得
		/// </summary>
		UINT GetNowFPS() const { return m_nowFPS; }

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
}