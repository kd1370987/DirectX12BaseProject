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
	Engine::MainEngine::Instance().Init();

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
		// プロファイラのフレーム開始
		Engine::Editor::MainEditor::Instance().BeginProfileFrame();

		Engine::Editor::MainEditor::Instance().StartTimer("MainLoop");
		Engine::Editor::MainEditor::Instance().StartTimer("MainLoop_Updatea");

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

		Engine::Editor::MainEditor::Instance().StopTimer("MainLoop_Updatea");

		Engine::Editor::MainEditor::Instance().StartTimer("MainLoop_Draw");

		// 描画
		Engine::MainEngine::Instance().BeginDraw();				// 描画開始
		{
			// ゲームの描画
			//App::Game::GameManager::Instance().Draw();
			// 命令の実行
			Engine::Editor::MainEditor::Instance().StartTimer("RGDraw");
			Engine::MainEngine::Instance().ExecuteDrawCmd();
			Engine::Editor::MainEditor::Instance().StopTimer("RGDraw");
		}
		Engine::MainEngine::Instance().EndDraw();					// 描画終了
		Engine::Editor::MainEditor::Instance().StopTimer("MainLoop_Draw");
		// フレーム終了
		Engine::MainEngine::Instance().EndFrame();
		Engine::Editor::MainEditor::Instance().StopTimer("MainLoop");

		// プロファイラのフレーム終了
		// ここで平均の確定と表示用の並べ替えが行われ、次フレームのパネル描画で使われる
		Engine::Editor::MainEditor::Instance().EndProfileFrame();
	}
}

Application::Application()
{
}

Application::~Application()
{
}
