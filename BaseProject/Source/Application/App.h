#pragma once

class Application
{
public:

	// アプリケーション実行
	void Execute();

private:

	// 初期化
	bool Init();

	// 解放
	void Release();

	// メインループ
	void MainLoop();

private:


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