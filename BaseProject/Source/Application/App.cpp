#include "App.h"

#include "Engine/Engine.h"

#include "Scene/SceneManager.h"

//==================================================================================
// 
// 初回呼び出し
// 
//==================================================================================
void Application::Excute()
{
	// アプリケーション初期化
	Init();

	// メインループ（更新処理・描画処理）
	MainLoop();

	// 解放
	Release();
}

//==================================================================================
// 
// アプリケーション初期化
// 
//==================================================================================
bool Application::Init()
{
	// エンジンの初期化
	EngineConfig _config;
	_config.graphics.init.isDebugLayer = true;
	_config.graphics.init.isGPUValidation = true;
	_config.graphics.runtime.isVsync = false;
	_config.app.mode = EngineConfig::Application::Mode::Debug;
	m_upEngine = std::make_unique<Engine>();
	m_upEngine->Init(_config);


	// シーンの初期化
	if (!SceneManager::Instance().Init())
	{
		assert(0 && "シーンマネージャの初期化に失敗");
		return false;
	}

	return true;
}

void Application::Release()
{
	
	// シーン解放
	SceneManager::Instance().Release();

	// エンジン解放
	m_upEngine->Release();
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
		// フレーム開始
		if (!m_upEngine->BegineFrame())
		{
			break;
		}

		// 更新
		SceneManager::Instance().Update(m_upEngine->GetDeltaTime());
		

		// 描画
		m_upEngine->BeginDraw();				// 描画開始
		{
			// 通常描画
			SceneManager::Instance().Draw();
		}
		m_upEngine->EndDraw();					// 描画終了

		// フレーム終了
		m_upEngine->EndFrame();
	}
}

Application::Application()
{
}

Application::~Application()
{
}
