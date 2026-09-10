#include "App.h"

#include "Engine/MainEngine.h"

#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/Input/InputManager/InputManager.h"
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
		//===========================================================================
		// このフレームぶんの計測
		//
		// 計測は ENGINE_PROFILE_SCOPE を置くだけ。スコープを抜けた時点で
		// 結果がエディター(Profiler)へ飛ぶので、集計する EndProfileFrame よりは
		// 内側で閉じておくこと
		//===========================================================================
		{
			ENGINE_PROFILE_SCOPE("MainLoop");

			{
				ENGINE_PROFILE_SCOPE("MainLoop_Update");

				// フレーム開始
				if (!Engine::MainEngine::Instance().BeginFrame())
				{
					break;
				}

				// モード切替
				ToggleAppMode();

				// ゲームの更新
				App::Game::GameManager::Instance().Update(Engine::MainEngine::Instance().GetDeltaTime());
			}

			{
				ENGINE_PROFILE_SCOPE("MainLoop_Draw");

				// 描画開始
				{
					ENGINE_PROFILE_SCOPE("BeginDraw");
					Engine::MainEngine::Instance().BeginDraw();
				}

				{
					// ゲームの描画
					//App::Game::GameManager::Instance().Draw();
					// 命令の実行
					ENGINE_PROFILE_SCOPE("RGDraw");
					Engine::MainEngine::Instance().ExecuteDrawCmd();
				}

				// 描画終了
				{
					ENGINE_PROFILE_SCOPE("EndDraw");
					Engine::MainEngine::Instance().EndDraw();
				}
			}

			// フレーム終了
			Engine::MainEngine::Instance().EndFrame();
		}

		// プロファイラのフレーム終了
		// ここで受け取った結果の集計・平均の確定・表示用の並べ替えが行われ、
		// 次フレームのパネル描画で使われる
		Engine::Editor::MainEditor::Instance().EndProfileFrame();
	}
}

//==================================================================================
//
// エディターとゲームの切り替え
//
//----------------------------------------------------------------------------------
// キーは InputManager が持っている(Ctrl+P / SYSTEM_ACTION_TOGGLE_APPMODE)。
// 割り当てを変えたいときはあちらを触ること。
//
//   エディター    → ゲーム
//   ゲーム        → エディター
//   デバッグプレイ → エディター(抜ける)
//
//==================================================================================
void Application::ToggleAppMode()
{
	// エディターがモーダルな画面(エフェクトエディター)を出している間は切り替えない。
	// あちらが開いている間はゲームのシーンが止まっているので、
	// ここで切り替えると「プレイモードなのに何も動かない」状態になってしまう
	if (Engine::Editor::MainEditor::Instance().IsModalActive()) return;

	// プレイモードでなくても拾う取り方。エディターに居るときに押すため
	if (!Engine::Input::InputManager::Instance().IsSystemPress(
		Engine::Input::InputManager::SYSTEM_ACTION_TOGGLE_APPMODE)) return;

	auto& _engine = Engine::MainEngine::Instance();

	// ゲームからでもデバッグプレイからでも、行き先はエディター
	const bool _isPlaying = (_engine.GetMode() != Engine::EAppMode::Editor);

	_engine.ChangeMode(_isPlaying ? Engine::EAppMode::Editor : Engine::EAppMode::Game);
}

Application::Application()
{
}

Application::~Application()
{
}
