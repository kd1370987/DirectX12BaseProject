#pragma once

class Engine;

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

	// エンジン
	std::unique_ptr<Engine> m_upEngine = nullptr;

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