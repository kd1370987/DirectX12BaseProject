#pragma once

namespace Engine::Time
{
	class FPSController;
	class DeltaTime;

	class TimeManager
	{
	public:

		TimeManager();
		~TimeManager();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="a_targetFPS">FPS制限</param>
		void Init(float a_targetFPS);

		/// <summary>
		/// 解放
		/// </summary>
		void Release();

		/// <summary>
		/// フレームの初めに計測
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// フレーム終わりに測定と、必要であればスリープ処理
		/// </summary>
		/// <param name="a_isVsync">垂直同期が有効かどうか</param>
		void EndFrame(bool a_isVsync);

		/// <summary>
		/// デルタタイム取得
		/// </summary>
		float GetDeltaTime() const;

		/// <summary>
		/// 現在のFPSを取得
		/// </summary>
		UINT GetNowFPS() const;

	private:

		std::unique_ptr<FPSController> m_upFPSController = nullptr;
		std::unique_ptr<DeltaTime> m_upDeltaTime = nullptr;

	};
}