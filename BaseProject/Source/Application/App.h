#pragma once

#include "Engine/Window/Window.h"

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

	// メインループ
	void MainLoop();
private:

	Window m_window;

// シングルトン
private:

	Application() {}
	~Application() {}

public:

	static Application& Instance()
	{
		static Application _instance;
		return _instance;
	}

};