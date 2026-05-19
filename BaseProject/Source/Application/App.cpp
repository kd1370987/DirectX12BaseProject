#include "App.h"

#include "Engine/MainEngine.h"

#include "Scene/SceneManager.h"

#include "../Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"
#include "../Engine/Resource/Loader/Model/ModelLoader.h"

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

const AppConfig& Application::GetConfig()
{
	return m_config;
}

//==================================================================================
// 
// アプリケーション初期化
// 
//==================================================================================
bool Application::Init()
{
	// エンジンの初期化
	Engine::EngineConfig _config;
	_config.graphics.init.isDebugLayer = true;
	_config.graphics.init.isGPUValidation = true;
	_config.graphics.runtime.isVsync = false;
	_config.app.mode = Engine::EngineConfig::Application::Mode::Debug;
	Engine::MainEngine::Instance().Init(_config);


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
	Engine::MainEngine::Instance().Release();
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
		Engine::Editor::MainEditor::Instance().StartWatch("MainLoop");
		Engine::Editor::MainEditor::Instance().StartWatch("MainLoop_Updatea");
		// フレーム開始
		if (!Engine::MainEngine::Instance().BegineFrame())
		{
			break;
		}

		// モード切替
		if (GetAsyncKeyState('O'))
		{
			Engine::MainEngine::Instance().ChangeMode(Engine::EngineConfig::Application::Mode::Debug);
		}
		if (GetAsyncKeyState('P'))
		{
			Engine::MainEngine::Instance().ChangeMode(Engine::EngineConfig::Application::Mode::Game);
		}


		// 更新
		SceneManager::Instance().Update(Engine::MainEngine::Instance().GetDeltaTime());
		Engine::Editor::MainEditor::Instance().EndWatch("MainLoop_Updatea");

		Engine::Editor::MainEditor::Instance().StartWatch("MainLoop_Draw");
		// 描画
		Engine::MainEngine::Instance().BeginDraw();				// 描画開始
		{
			// 通常描画
			SceneManager::Instance().Draw(Engine::MainEngine::Instance().RefRenderContext());
		}
		Engine::MainEngine::Instance().EndDraw();					// 描画終了

		// フレーム終了
		Engine::MainEngine::Instance().EndFrame();
		Engine::Editor::MainEditor::Instance().EndWatch("MainLoop_Draw");
		Engine::Editor::MainEditor::Instance().EndWatch("MainLoop");
	}
}

Application::Application()
{
}

Application::~Application()
{
}
