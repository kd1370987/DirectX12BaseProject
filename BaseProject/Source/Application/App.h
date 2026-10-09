#pragma once

namespace App::Game
{
	class GameManager;
}

namespace App
{
	// アプリの寿命そのもの。ゲーム(GameManager)の持ち主
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

	private:

		// ゲーム全体の流れ : エンジンの初期化の後に作り、エンジンの解放より前に手放す
		std::unique_ptr<Game::GameManager> m_upGameManager = nullptr;
	};
}