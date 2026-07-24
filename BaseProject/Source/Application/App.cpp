#include "App.h"

#include "Engine/MainEngine.h"

#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Game/GameManager/GameManager.h"

#include "../Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"


//==================================================================================
// 
// 初回呼び出し
// 
//==================================================================================
void Application::Execute()
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

	// ゲームの初期化
	App::Game::GameManager::Instance().Init();

	return true;
}

void Application::Release()
{
	
	// シーン解放
	Engine::Scene::SceneManager::Instance().Release();

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
		if (!Engine::MainEngine::Instance().BeginFrame())
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

		// ゲームの更新
		App::Game::GameManager::Instance().Update(Engine::MainEngine::Instance().GetDeltaTime());

		Engine::Editor::MainEditor::Instance().EndWatch("MainLoop_Updatea");

		Engine::Editor::MainEditor::Instance().StartWatch("MainLoop_Draw");

		// 描画
		Engine::MainEngine::Instance().BeginDraw();				// 描画開始
		{
			// ゲームの描画
			//App::Game::GameManager::Instance().Draw();
			// 命令の実行
			Engine::Editor::MainEditor::Instance().StartWatch("RGDraw");
			Engine::MainEngine::Instance().ExecuteDrawCmd();
			Engine::Editor::MainEditor::Instance().EndWatch("RGDraw");
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
