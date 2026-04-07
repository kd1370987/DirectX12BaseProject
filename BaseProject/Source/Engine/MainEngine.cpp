#include "MainEngine.h"

#include "Engine/Window/NativeWindow.h"
#include "Engine/TimeManager/TimeManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "Application/App.h"

namespace Engine
{
	MainEngine::MainEngine()
	{}

	MainEngine::~MainEngine()
	{}

	void MainEngine::Init(EngineConfig a_config)
	{
		// ウィンドウ設定取得
		auto _windowWidth = Application::Instance().GetConfig().windowWidth;
		auto _windowHeight = Application::Instance().GetConfig().windowHegiht;

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
		m_upWindow = std::make_unique<Window::NativeWindow>();
		Window::WindowDesc _desc = {};
		_desc.width = _windowWidth;
		_desc.height = _windowHeight;
		_desc.titleName = L"DirectX12";
		_desc.className = L"AppWindow";
		_desc.windowMode = Window::EWindowMode::Windowed;
		if (!m_upWindow->Create(_desc))
		{
			assert(0 && "ウィンドウ作成失敗");
			return;
		}

		// タイムマネージャークラスの生成
		m_upTimeManager = std::make_unique<TimeManager>();
		m_upTimeManager->Init(120.0f);

		// DirectX12関連オブジェクトの初期化
		if (!D3D12Wrapper::Instance().Init(m_upWindow->GetWindowHandle(), m_upWindow->GetClientWidth(), m_upWindow->GetClientHeight()))
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
		Engine::Graphics::RenderContext::Instance().Init();

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
		Engine::Graphics::RenderContext::Instance().Shutdown();

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
		if (m_config.graphics.init.isDebugLayer)
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
		Engine::Graphics::RenderContext::Instance().BeginFrame();

		auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
				DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap()
		};
		_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);
	}

	void MainEngine::EndDraw()
	{
		auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

		Engine::Raytracing::RayEngine::Instance().Dispatch();

		// ゲームモード以外の処理
		if (m_config.app.mode != EngineConfig::Application::Mode::Game)
		{
			// ディスクリプタヒープをセット
			ID3D12DescriptorHeap* _heaps[] = {
					DescriptorHeapManager::Instance().GetImGuiHeap()
			};
			_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);
			// エディター描画
			ImGuiContex::Instance().CallImGuiDrawData(D3D12Wrapper::Instance().GetCommandList());
		}
		// ディスクリプタヒープをセット
		ID3D12DescriptorHeap* _heaps[] = {
				DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap()
		};
		_pCmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

		// 描画フレームリソース
		Engine::Graphics::RenderContext::Instance().EndFrame();

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
}