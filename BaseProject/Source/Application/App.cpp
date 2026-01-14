#include "App.h"

#include "Engine/Window/Window.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Scene/SceneManager.h"

#include "Engine/GPUResource/Model/Model.h"
#include "Engine/TimeManager/TimeManager.h"

#include "Core/Time/FPSController/FPSController.h"

// ECS
#include "Engine/ECS/World/World.h"

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
	m_upWindow = std::make_unique<Window>();
	if (!m_upWindow->Create(WINDOW_WIDTH, WINDOW_HEIGHT, L"DirectX12", L"Window"))
	{
		assert(0 && "ウィンドウ作成失敗");
		return false;
	}

	// タイムマネージャー作成
	m_upTimeManager = std::make_unique<TimeManager>();
	m_upTimeManager->Init(60.0f);
	
	// 描画エンジンの初期化
	if (!RenderingEngine::Instance().Init(m_upWindow->GetWindowHandle(), WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		assert(0 && "描画エンジンの初期化に失敗");
		return false;
	}

	// 描画初期化
	RenderContext::Instance().Init();

	// ECSの初期化
	World::Instance().Init();

	// シーンの初期化
	if (!SceneManager::Instance().Init())
	{
		return false;
	}

	// 垂直同期
	m_isVsync = true;

	return true;
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
		m_upTimeManager->BeginFrame();

		// メッセージ処理
		if (!m_upWindow->ProcessMessage())
		{
			break;
		}

		// タイトル文字列変更
		std::string _str = "FPS = " + std::to_string(m_upTimeManager->GetNowFPS()) + 
			"; DELTATIME = " + std::to_string(m_upTimeManager->GetDeltaTime()) + ";";
		m_upWindow->ChangeTitle(_str);

		// 更新
		SceneManager::Instance().Update(m_upTimeManager->GetDeltaTime());
		

		// 描画
		RenderingEngine::Instance().BeginRender();			// 描画開始
		{
			SceneManager::Instance().Draw();				// 描画
		}
		RenderingEngine::Instance().EndRender(m_isVsync);	// 描画終了

		// フレーム終了
		m_upTimeManager->EndFrame(m_isVsync);
	}
}

Application::Application()
{
}

Application::~Application()
{
}
