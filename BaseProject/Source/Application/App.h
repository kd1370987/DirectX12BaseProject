#pragma once

class Application
{
public:

	Application();
	~Application();

	// アプリケーション実行
	void Execute();

private:

	// 初期化
	bool Init();

	// 解放
	void Release();

	// メインループ
	void MainLoop();

	// エディターとゲームの切り替え(Ctrl+P)
	void ToggleAppMode();

};