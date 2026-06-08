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

//==================================================================================
// 
// アプリケーション初期化
// 
//==================================================================================
bool Application::Init()
{
	// エンジンの初期化
	Engine::InitConfig  _initConfig = {};
	_initConfig.buildMode = Engine::EBuildConfiguration::Shipping;
	_initConfig.assetRootPath = "Asset/";
	_initConfig.isDebugLayer = true;
	_initConfig.isGPUValidation = true;
	_initConfig.maxThreadCount = 4;
	Engine::RuntimeConfig _runtimeConfig = {};
	_runtimeConfig.appMode = Engine::EAppMode::Editor;
	_runtimeConfig.mainSoundBolume = 50;
	Engine::EngineConfig _config;
	_config.Init(_initConfig,_runtimeConfig);
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
			Engine::MainEngine::Instance().ChangeMode(Engine::EAppMode::Editor);
		}
		if (GetAsyncKeyState('P'))
		{
			Engine::MainEngine::Instance().ChangeMode(Engine::EAppMode::Game);
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
		Engine::Editor::MainEditor::Instance().EndWatch("MainLoop_Draw");
		// フレーム終了
		Engine::MainEngine::Instance().EndFrame();
		Engine::Editor::MainEditor::Instance().EndWatch("MainLoop");
	}
}

Application::Application()
{
}

Application::~Application()
{
}
