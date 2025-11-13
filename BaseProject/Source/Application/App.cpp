#include "App.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/GameScene/SceneManager/SceneManager.h"

//==================================================================================
// 
// 初回呼び出し
// 
//==================================================================================
void Application::Excute()
{
	// アプリケーション初期化
	if (!Init())
	{
		assert(0 && "アプリケーション初期化に失敗");
		return;
	}

	// メインループ（更新処理・描画処理）
	MainLoop();
}

//==================================================================================
// 
// アプリケーション初期化
// 
//==================================================================================
bool Application::Init()
{
	// ウィンドウの生成（ここの変数も外部からとってきたい）
	if (!m_window.Create(WINDOW_WIDTH, WINDOW_HEIGHT, L"DirectX12", L"Window"))
	{
		assert(0 && "ウィンドウ作成失敗");
		return false;
	}
	// 描画エンジンの初期化
	if (!RenderingEngine::Instance().Init(m_window.GetWindowHandle(), WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		assert(0 && "描画エンジンの初期化に失敗");
		return false;
	}
	// シーンの初期化
	if (!SceneManager::Instance().Init())
	{
		return false;
	}
}

//==================================================================================
// 
// メインループ
// 
//==================================================================================
void Application::MainLoop()
{
	while (true)
	{
		// メッセージ処理
		if (!m_window.ProcessMessage())
		{
			break;
		}

		// 更新
		SceneManager::Instance().Update();
		

		// 描画
		RenderingEngine::Instance().BeginRender();			// 描画開始
		{
			SceneManager::Instance().Draw();				// 描画
		}
		RenderingEngine::Instance().EndRender();			// 描画終了
	}
}
