#pragma once

class Window;
class TimeManager;

class ImGuiContex;

constexpr UINT WINDOW_WIDTH = 1280;
constexpr UINT WINDOW_HEIGHT = 720;

class Application
{
public:


	/// <summary>
	/// アプリケーション実行
	/// </summary>
	/// <returns>初期化に成功したらtrue</returns>
	void Excute();

private:

	// 初期化
	bool Init();

	// 解放
	void Release();

	// メインループ
	void MainLoop();

private:

	std::unique_ptr<Window>			m_upWindow			= nullptr;
	std::unique_ptr<TimeManager>	m_upTimeManager		= nullptr;

	bool m_isVsync = false;

// シングルトン
private:

	Application();
	~Application();

public:

	static Application& Instance()
	{
		static Application _instance;
		return _instance;
	}

};