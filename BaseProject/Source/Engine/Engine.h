#pragma once

class Window;
class TimeManager;

class Engine
{
public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 解放処理
	/// </summary>
	void Release();

	/// <summary>
	/// フレームの初めに呼び出す
	/// </summary>
	void BegineFrame();

	/// <summary>
	/// フレームの終わりに呼び出す
	/// </summary>
	void EndFrame();

private:

	std::unique_ptr<Window> m_upWindow = nullptr;
	std::unique_ptr<TimeManager> m_upTimeManager = nullptr;

	bool m_isVsync = false;

	bool m_isDebug = false;
};
