#pragma once

namespace Engine
{
	class MainEngine;
}

struct AppConfig
{
	// フルスクリーンかどうか
	bool isFullScreen = false;

	// ウィンドウサイズ
	float windowWidth = 1280;
	float windowHegiht = 720;
};

class Application
{
public:


	/// <summary>
	/// アプリケーション実行
	/// </summary>
	/// <returns>初期化に成功したらtrue</returns>
	void Excute();

	/// <summary>
	/// アプリケーション設定を取得
	/// </summary>
	const AppConfig& GetConfig();

private:

	// 初期化
	bool Init();

	// 解放
	void Release();

	// メインループ
	void MainLoop();

private:

	// エンジン
	std::unique_ptr<Engine::MainEngine> m_upEngine = nullptr;

	// アプリケーション設定
	AppConfig m_config = {};

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