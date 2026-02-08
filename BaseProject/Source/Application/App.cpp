#include "App.h"

#include "Engine/Window/Window.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#include "Scene/SceneManager.h"

#include "Engine/TimeManager/TimeManager.h"

#include "Core/Time/FPSController/FPSController.h"

// ECS
#include "Engine/ECS/World/World.h"

// デバッグ・エディター
#include "Engine/Editor/ImGui/ImGuiContext.h"

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
	// DX12デバッグワイヤーオン
#ifdef _DEBUG
	{
		ComPtr<ID3D12Debug> _debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debug))))
		{
			_debug->EnableDebugLayer();
		}
	}
#endif

	// ウィンドウの生成（ここの変数も外部からとってきたい）
	m_upWindow = std::make_unique<Window>();
	if (!m_upWindow->Create(WINDOW_WIDTH, WINDOW_HEIGHT, L"DirectX12", L"Window"))
	{
		assert(0 && "ウィンドウ作成失敗");
		return false;
	}

	// タイムマネージャー作成
	m_upTimeManager = std::make_unique<TimeManager>();
	m_upTimeManager->Init(120.0f);
	
	// DirectX12関係の初期化
	if (!D3D12Wrapper::Instance().Init(m_upWindow->GetWindowHandle(), WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		assert(0 && "描画エンジンの初期化に失敗");
		return false;
	}

	// ImGui初期化
	if (!ImGuiContex::Instance().Init(m_upWindow->GetWindowHandle()))
	{
		assert(0 && "ImGuiの初期化に失敗");
		return false;
	}

	// 描画初期化
	RenderContext::Instance().Init();

	// リソースマネージャーの初期化
	GraphicResourceManager::Instance().Init();

	// ECSの初期化
	World::Instance().Init();

	// シーンの初期化
	if (!SceneManager::Instance().Init())
	{
		assert(0 && "シーンマネージャの初期化に失敗");
		return false;
	}

	// 垂直同期
	m_isVsync = false;

	return true;
}

void Application::Release()
{
	// シーン解放
	SceneManager::Instance().Release();

	// ECS解放
	World::Instance().Release();

	// GPU同期待ち
	for (UINT _i = 0; _i < static_cast<UINT>(CPU_FRAME_COUNT); ++_i)
	{
		D3D12Wrapper::Instance().WaitRender(_i);
	}


	// グラフィックリソースの解放
	GraphicResourceManager::Instance().Release();

	// レンダーコンテキスト解放
	RenderContext::Instance().Shutdown();

	// ImGui解放
	ImGuiContex::Instance().Release();

	// 描画エンジン解放
	D3D12Wrapper::Instance().Shutdown();

	// タイムマネージャー解放
	m_upTimeManager->Release();

	// 解放時にエラー検出
#if _DEBUG
	{
		ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(
				DXGI_DEBUG_ALL,
				DXGI_DEBUG_RLO_DETAIL
			);
		}
	}
#endif

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
		std::string _str = "DX12_FrameWork FPS = " + std::to_string(m_upTimeManager->GetNowFPS()) +
			"; DELTATIME = " + std::to_string(m_upTimeManager->GetDeltaTime()) + ";";
		m_upWindow->ChangeTitle(_str);

		// 更新
		SceneManager::Instance().Update(m_upTimeManager->GetDeltaTime());
		

		// 描画
		D3D12Wrapper::Instance().BeginFrame();			// 描画開始
		{
			// 通常描画
			SceneManager::Instance().Draw();

			// ImGui描画
			ImGuiContex::Instance().CallImGuiDrawData(D3D12Wrapper::Instance().GetCommandList());
		}
		D3D12Wrapper::Instance().EndFrame(m_isVsync);	// 描画終了

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
