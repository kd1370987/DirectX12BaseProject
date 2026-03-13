#include "Engine.h"

#include "Engine/Window/Window.h"
#include "Engine/TimeManager/TimeManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "Application/App.h"

MainEngine::MainEngine()
{
}

MainEngine::~MainEngine()
{
}

void MainEngine::Init(EngineConfig a_config)
{
	// ウィンドウ設定取得
	m_windowWidth = Application::Instance().GetConfig().windowWidth;
	m_windowHeight = Application::Instance().GetConfig().windowHegiht;

	// DirectX12でGPUの詳細なエラーを確認するためのもの
	if (a_config.graphics.init.isDebugLayer)
	{
		ComPtr<ID3D12Debug> _debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debug))))
		{
			_debug->EnableDebugLayer();
		}
	}

	// ウィンドウクラスの生成
	m_upWindow = std::make_unique<Window>();
	if (!m_upWindow->Create(m_windowWidth, m_windowHeight, L"DirectX12", L"Window"))
	{
		assert(0 && "ウィンドウ作成失敗");
		return;
	}

	// タイムマネージャークラスの生成
	m_upTimeManager = std::make_unique<TimeManager>();
	m_upTimeManager->Init(120.0f);

	// DirectX12関連オブジェクトの初期化
	if (!D3D12Wrapper::Instance().Init(m_upWindow->GetWindowHandle(), m_windowWidth, m_windowHeight))
	{
		assert(0 && "D3D12Wrapperの初期化失敗");
		return;
	}

	// ディスクリプタヒープテーブルマネージャーの初期化
	if (!DescriptorHeapManager::Instance().Init())
	{
		assert(0 && "ディスクリプタヒープマネージャーの初期化に失敗");
		return;
	}

	// バックバッファの生成
	if (!D3D12Wrapper::Instance().CreateRenderTarget())
	{
		assert(0 && "バックバッファの生成に失敗");
		return;
	}

	// エディター初期化
	if (!ImGuiContex::Instance().Init(m_upWindow->GetWindowHandle()))
	{
		assert(0 && "エディターの初期化に失敗");
		return;
	}

	// 描画初期化
	RenderContext::Instance().Init();

	// レイエンジン初期化
	Engine::Raytracing::RayEngine::Instance().Create();

	m_config = a_config;
}

void MainEngine::Release()
{
	// GPU同期待ち
	for (UINT _i = 0; _i < static_cast<UINT>(CPU_FRAME_COUNT); ++_i)
	{
		D3D12Wrapper::Instance().WaitRender(_i);
	}


	// レンダーコンテキスト解放
	RenderContext::Instance().Shutdown();

	// ImGui解放
	ImGuiContex::Instance().Release();

	// ディスクリプタヒープマネージャー解放
	DescriptorHeapManager::Instance().Release();

	// 描画エンジン解放
	D3D12Wrapper::Instance().Shutdown();

	// タイムマネージャー解放
	m_upTimeManager->Release();

	// ウィンドウの解放
	m_upWindow->Release();

	// 解放時にエラー検出
	if(m_config.graphics.init.isDebugLayer)
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
}

bool MainEngine::BegineFrame()
{
	// フレーム開始
	m_upTimeManager->BeginFrame();

	// メッセージ処理
	if (!m_upWindow->ProcessMessage())
	{
		return false;
	}

	// タイトル文字列変更
	std::string _str = "DX12_FrameWork FPS = " + std::to_string(m_upTimeManager->GetNowFPS()) +
		"; DELTATIME = " + std::to_string(m_upTimeManager->GetDeltaTime()) + ";";
	m_upWindow->ChangeTitle(_str);

	return true;
}

void MainEngine::EndFrame()
{
	// フレーム終了
	m_upTimeManager->EndFrame(m_config.graphics.runtime.isVsync);
}

void MainEngine::BeginDraw()
{
	// 描画開始
	D3D12Wrapper::Instance().BeginFrame();

	// 描画フレームリソース
	RenderContext::Instance().BeginFrame();
}

void MainEngine::EndDraw()
{
	// ゲームモード以外の処理
	if (m_config.app.mode != EngineConfig::Application::Mode::Game)
	{
		// エディター描画
		ImGuiContex::Instance().CallImGuiDrawData(D3D12Wrapper::Instance().GetCommandList());
	}

	// 描画フレームリソース
	RenderContext::Instance().EndFrame();

	// 描画終了
	D3D12Wrapper::Instance().EndFrame(m_config.graphics.runtime.isVsync);
}

float MainEngine::GetDeltaTime()
{
	return m_upTimeManager->GetDeltaTime();
}

void MainEngine::ChangeMode(EngineConfig::Application::Mode a_mode)
{
	m_config.app.mode = a_mode;
}
