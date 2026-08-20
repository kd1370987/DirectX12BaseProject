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
		//
		// GetAsyncKeyState は押している間ずっと真を返し、しかもどこにフォーカスが
		// あっても拾ってしまう。押した瞬間だけを見て、さらにエディターで
		// 文字を打っている間は無視する(名前に O/P が入るたびに切り替わるため)。
		{
			const bool _isTyping =
				(ImGui::GetCurrentContext() != nullptr) && ImGui::GetIO().WantTextInput;

			// エディターがモーダルな画面(エフェクトエディター)を出している間は切り替えない。
			// あちらが開いている間はゲームのシーンが止まっているので、
			// ここで切り替えると「プレイモードなのに何も動かない」状態になってしまう
			const bool _isModal = Engine::Editor::MainEditor::Instance().IsModalActive();

			// 押した瞬間の検出用。押しっぱなしで連続発火させない
			static bool s_wasEditorKeyDown = false;
			static bool s_wasGameKeyDown   = false;

			const bool _isEditorKeyDown = (GetAsyncKeyState('O') & 0x8000) != 0;
			const bool _isGameKeyDown   = (GetAsyncKeyState('P') & 0x8000) != 0;

			if (!_isTyping && !_isModal)
			{
				if (_isEditorKeyDown && !s_wasEditorKeyDown)
				{
					Engine::MainEngine::Instance().ChangeMode(Engine::EAppMode::Editor);
				}
				if (_isGameKeyDown && !s_wasGameKeyDown)
				{
					Engine::MainEngine::Instance().ChangeMode(Engine::EAppMode::Game);
				}
			}

			s_wasEditorKeyDown = _isEditorKeyDown;
			s_wasGameKeyDown   = _isGameKeyDown;
		}

		// ゲームの更新
		App::Game::GameManager::Instance().Update(Engine::MainEngine::Instance().GetDeltaTime());

		Engine::Editor::MainEditor::Instance().StopTimer("MainLoop_Updatea");

		Engine::Editor::MainEditor::Instance().StartTimer("MainLoop_Draw");

		// 描画
		Engine::Editor::MainEditor::Instance().StartTimer("BeginDraw");
		Engine::MainEngine::Instance().BeginDraw();				// 描画開始
		Engine::Editor::MainEditor::Instance().StopTimer("BeginDraw");
		{
			// ゲームの描画
			//App::Game::GameManager::Instance().Draw();
			// 命令の実行
			Engine::Editor::MainEditor::Instance().StartTimer("RGDraw");
			Engine::MainEngine::Instance().ExecuteDrawCmd();
			Engine::Editor::MainEditor::Instance().StopTimer("RGDraw");
		}
		Engine::Editor::MainEditor::Instance().StartTimer("EndDraw");
		Engine::MainEngine::Instance().EndDraw();						// 描画終了
		Engine::Editor::MainEditor::Instance().StopTimer("EndDraw");
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
