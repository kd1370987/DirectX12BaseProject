#include "D3D12Wrapper.h"

#include "../Command/CommandContext/CommandContext.h"
#include "../Command/CommandPool/CommandPool.h"

#include "../FrameManager/FrameManager.h"

#include "../AsyncGPUManager/AsyncGPUManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::D3D12
{
	void D3D12Wrapper::Init(const HWND& a_hWnd, UINT a_windowWidth, UINT a_windowHeight)
	{
		// デバッグ機能を有効化(終了時のライブオブジェクトレポート等で使用)
		m_isDebag = true;

		// GPUリソース初期化
		CreateDxgiFactory();	// ファクトリ作成
		FindAdapter();			// アダプター検索	
		CreateDevice();			// デバイス作成

		// コマンドコンテキスト作成
		CreateCommandContext();

		// 非同期マネージャー作成
		CreateAsyncGPUManager();

		// フレームマネージャー作成
		CreateFrameManager();

		// バッファリング関係
		CreateSwapChain(a_hWnd,a_windowWidth,a_windowHeight);		// スワップチェイン作成
		CreateViewPort(a_windowWidth, a_windowHeight);				// 描画用領域設定
		CreateScissorRect(a_windowWidth, a_windowHeight);			// 描画範囲作成

		ENGINE_LOG("D3D12Wrapper作成");
	}

	void D3D12Wrapper::Release()
	{
		// GPU待機処理
		m_upFrameManager->Release();

		// 非同期マネージャー解放
		m_upAsyncGPUManager->Release();

		// コマンドリスト解放
		m_upCommandContext->RefDirectPool()->Release();
		m_upCommandContext->RefCopyPool()->Release();
		m_upCommandContext->RefComputePool()->Release();

		// バックバッファ解放
		m_pCurrentRenderTarget = nullptr;
		for (auto& _tex : m_backBuffers)
		{
			_tex.Release();
		}

		// DX12オブジェクト解放
		m_cpSwapChain.Reset();
		m_cpAdapter.Reset();
		m_cpFactory.Reset();

		// リーク調査 : デバイスがまだ生きているこのタイミングで
		// ID3D12DebugDevice のレポートを出す。
		// DXGI の ReportLiveObjects と違い、こちらは SetName で付けた名前を表示するので、
		// どのリソースが残っているか特定できる。
		// D3D12_RLDO_IGNORE_INTERNAL を付けて、ランタイム内部の参照だけのオブジェクトは除外する。
		if (m_isDebag)
		{
			ComPtr<ID3D12DebugDevice> _debDev;
			if (SUCCEEDED(m_cpDevice->QueryInterface(IID_PPV_ARGS(&_debDev))))
			{
				_debDev->ReportLiveDeviceObjects(
					D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
			}
		}

		// 最後にデバイスを解放
		m_cpDevice.Reset();
	}

	//==================================================================================
	// 
	// 描画開始・描画終了
	// 
	//==================================================================================
	void D3D12Wrapper::BeginFrame()
	{
		// バックバッファ番号更新
		m_currentBackBufferIndex = m_cpSwapChain->GetCurrentBackBufferIndex();

		// フレームインデックス更新 : GPU待機
		m_upFrameManager->BeginFrame();
	}
	void D3D12Wrapper::EndFrame(bool a_isVsync)
	{

		auto* _pBarrierCmdList = GetDirectCommandList();
		// レンダーターゲットに書き込みが終わるまで待つ
		ResourceBarrier(
			_pBarrierCmdList,
			m_backBuffers[m_currentBackBufferIndex].GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);

		// コマンドリストを実行待ちに追加
		SubmitDirectCommandList(_pBarrierCmdList);

		// コマンドリストの実行
		m_upCommandContext->RefDirectPool()->ExecutePendingLists();

		// フレーム終了シグナル
		m_upFrameManager->EndFrame(m_upCommandContext->RefDirectPool()->GetCommandQueue());

		// スワップチェイン切替
		m_cpSwapChain->Present(a_isVsync ? 1 : 0, 0);
	}

	void D3D12Wrapper::ExecuteAsyncCompute(std::function<void(GraphicsCommandList*)> a_recordCmds, std::function<void()> a_onComplete)
	{
		// 処理の流れはCopyと全く同じで、Compute用のプールとタイプを指定します
		auto* _allocator = m_upAsyncGPUManager->AcquireAllocator(m_cpDevice.Get(), AsyncCommandType::Compute);
		GraphicsCommandList* _cmdList = m_upCommandContext->RefComputePool()->AcquireList(m_cpDevice.Get(), _allocator);

		if (a_recordCmds) {
			a_recordCmds(_cmdList);
		}

		m_upCommandContext->RefComputePool()->SubmitList(_cmdList);
		UINT64 _fenceValue = m_upCommandContext->RefComputePool()->ExecutePendingLists();

		m_upAsyncGPUManager->RegisterTask(
			AsyncCommandType::Compute,
			_allocator,
			m_upCommandContext->RefComputePool()->GetFence(),
			_fenceValue,
			a_onComplete
		);
	}

	void D3D12Wrapper::WaitForAsyncTasks()
	{
	}

	void D3D12Wrapper::WaitForFrame()
	{
		m_upFrameManager->WaitForAll();
	}

	//==================================================================================
	// 
	// ゲッター
	// 
	//==================================================================================
	// GPU取得
	Adapter* D3D12Wrapper::GetDXGIAdapter()
	{
		return m_cpAdapter.Get();
	}

	// デバイス取得
	Device* D3D12Wrapper::GetDevice()
	{
		return m_cpDevice.Get();
	}
		
	// 現在のバックバッファ番号取得
	UINT D3D12Wrapper::CurrentBackBufferIndex()
	{
		return m_currentBackBufferIndex;
	}

	// 現在のCPUフレーム番号取得
	UINT D3D12Wrapper::CurrentCPUFrameIndex()
	{
		return m_upFrameManager->GetCPUFrameIndex();
	}

	// バックバッファ取得
	ID3D12Resource* D3D12Wrapper::GetCurrentBackBuffer()
	{
		return m_backBuffers[m_currentBackBufferIndex].GetResource();
	}

	const Resource::Texture& D3D12Wrapper::GetCurrentBackBufferTex() const
	{
		return m_backBuffers[m_currentBackBufferIndex];
	}

	// コマンドキュー取得
	ID3D12CommandQueue* D3D12Wrapper::GetCommandQueue()
	{
		return m_upCommandContext->RefDirectPool()->GetCommandQueue();
	}
	ID3D12CommandQueue* D3D12Wrapper::GetCopyCommandQueue()
	{
		return m_upCommandContext->RefCopyPool()->GetCommandQueue();
	}
	ID3D12CommandQueue* D3D12Wrapper::GetComputeCommandQueue()
	{
		return m_upCommandContext->RefComputePool()->GetCommandQueue();
	}

	// コマンドリスト取得
	GraphicsCommandList* D3D12Wrapper::GetDirectCommandList()
	{
		return m_upCommandContext->RefDirectPool()->AcquireList(
			m_cpDevice.Get(),
			m_upFrameManager->GetCurrentAllocator()
		);
	}
	UINT64 D3D12Wrapper::GetCurrentFenceValue()
	{
		return m_upFrameManager->GetCurrentFenceValue();
	}
	UINT64 D3D12Wrapper::GetCompletedFenceValue()
	{
		return m_upFrameManager->GetCompletedFenceValue();
	}
	UINT64 D3D12Wrapper::GetNextFenceValue()
	{
		return m_upFrameManager->GetNextFenceValue();
	}
	//==================================================================================
	// 
	// D3D12オブジェクト作成
	// 
	//==================================================================================
	void D3D12Wrapper::CreateDxgiFactory()
	{
		UINT _flgsDXGI = 0;
		if (m_isDebag)
		{
			// デバッグレイヤーを有効化
			_flgsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
			ENGINE_LOG("DXGIFactoryのデバッグレイヤーON");
		}
		// DXGIファクトリの生成
		HRESULT _hr = CreateDXGIFactory2(
			_flgsDXGI,
			IID_PPV_ARGS(m_cpFactory.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(FAILED(_hr), "DXGIファクトリの生成に失敗");
			return;
		}
	}
	void D3D12Wrapper::FindAdapter()
	{
		m_cpAdapter = nullptr;								// クリア
		std::vector<ComPtr<IDXGIAdapter>> _pAdapters;		// 発見したアダプタ群
		std::vector<DXGI_ADAPTER_DESC> _adapterDescs;		// アダプタの説明群

		// 使用中PCにあるGPUドライバーを検索して、アダプタを列挙
		for (UINT _i = 0; 1; ++_i)
		{
			// デバイスを検索し、見つかれば格納
			_pAdapters.push_back(nullptr);
			HRESULT _hr = m_cpFactory->EnumAdapters(_i, &_pAdapters[_i]);

			// デバイスが見つからなければ終了
			if (_hr == DXGI_ERROR_NOT_FOUND) break;

			// 見つけたデバイスの説明を取得
			_adapterDescs.push_back({});
			_pAdapters[_i]->GetDesc(&_adapterDescs[_i]);
		}

		// 優先度の高いGPUドライバーを使用する
		GPUTier _guiTier = GPUTier::Kind;
		for (int _i = 0; _i < _adapterDescs.size(); ++_i)
		{
			if (std::wstring(_adapterDescs[_i].Description).find(L"NVIDIA") != std::wstring::npos)
			{
				// NVIDIAが見つかったら即決
				m_cpAdapter = _pAdapters[_i];
				break;
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Amd") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Amd)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Amd;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Intel") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Intel)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Intel;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Arm") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Arm)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Arm;
				}
			}
			else if (std::wstring(_adapterDescs[_i].Description).find(L"Qualcomm") != std::wstring::npos)
			{
				// 選択中のGPUがAmdより低優先度なら入れ替え
				if (_guiTier > GPUTier::Qualcomm)
				{
					m_cpAdapter = _pAdapters[_i];
					_guiTier = GPUTier::Qualcomm;
				}
			}
		}
	}
	void D3D12Wrapper::CreateDevice()
	{
		// 検索用レベル
		D3D_FEATURE_LEVEL _levels[] =
		{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};

		// デバイスの生成
		HRESULT _hr = S_FALSE;
		for (auto _level : _levels)
		{
			// 生成可能なレベルで試す
			_hr = D3D12CreateDevice(
				m_cpAdapter.Get(),
				_level,
				IID_PPV_ARGS(m_cpDevice.ReleaseAndGetAddressOf())
			);
			if (SUCCEEDED(_hr))
			{
				// 成功したら抜ける
				break;
			}
		}
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(FAILED(_hr), "デバイス生成に失敗");
			return;
		}
		

		// DynamicResourceBindが対応されているかのチェック
		m_isDynamicResourceSupported = false;

		D3D12_FEATURE_DATA_D3D12_OPTIONS	_featureOptions = {};
		D3D12_FEATURE_DATA_SHADER_MODEL		_shaderModel = {};
		_shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		if (SUCCEEDED(m_cpDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &_featureOptions, sizeof(_featureOptions)))
			&& SUCCEEDED(m_cpDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &_shaderModel, sizeof(_shaderModel))))
		{
			bool _isTier3 = _featureOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3;
			m_isDynamicResourceSupported = _isTier3;
		}
		ENGINE_ERRLOG(m_isDynamicResourceSupported,"動的リソースがサポートされていないGPUが選択されました");

		// デバッグ設定
		if (m_isDebag)
		{
			ComPtr<ID3D12DebugDevice> _debDev;
			if (SUCCEEDED(m_cpDevice->QueryInterface(IID_PPV_ARGS(&_debDev))))
			{
				_debDev->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
			}
			ENGINE_LOG("Dviceのデバッグ設定ON");
		}

		return;
	}

	void D3D12Wrapper::CreateSwapChain(HWND a_hWnd,UINT a_windowWidth, UINT a_windowHeight)
	{
		// テレイングチェック
		m_cpFactory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&m_isAllowTearing,
			sizeof(m_isAllowTearing)
		);

		// 仕様書作成
		DXGI_SWAP_CHAIN_DESC1 _desc = {};
		_desc.Width = a_windowWidth;							// 横幅
		_desc.Height = a_windowHeight;							// 高さ
		_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// ピクセルのフォーマット
		_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// バッファの用途（出力）
		_desc.BufferCount = BACKBUFFER_COUNT;					// バッファ数(ダブルバッファリング、トリプルバッファリング)
		_desc.SampleDesc.Count = 1;								// マルチサンプリング（なし）
		_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// 切替の方式
		_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		_desc.Scaling = DXGI_SCALING_STRETCH;
		_desc.Flags = m_isAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;	// 可変フレームレート

		// キュー取得
		auto* _pCmdQueue = m_upCommandContext->RefDirectPool()->GetCommandQueue();
		ENGINE_ERRLOG(_pCmdQueue, "コマンドキューの取得に失敗");

		// スワップチェインの作成
		ComPtr<IDXGISwapChain1> _swapChain1;
		HRESULT _hr = m_cpFactory->CreateSwapChainForHwnd(
			_pCmdQueue,
			a_hWnd,
			&_desc,
			nullptr,
			nullptr,
			&_swapChain1
		);
		ENGINE_ERRLOG(SUCCEEDED(_hr), "スワップチェインの作成失敗");

		// コピー
		_swapChain1.As(&m_cpSwapChain);
		_swapChain1.Reset();

		// バックバッファ番号を取得
		m_currentBackBufferIndex = m_cpSwapChain->GetCurrentBackBufferIndex();
	}

	void D3D12Wrapper::CreateViewPort(UINT a_windowWidth, UINT a_windowHeight)
	{
		// ビューポート
		// ウィンドウに対してレンダリング結果をどう表示するかの設定
		// 左上座標
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;

		// 幅・高さ
		m_viewport.Width = a_windowWidth;
		m_viewport.Height = a_windowHeight;

		// 深度のマッピング範囲（奥行情報・Zバッファの値）
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;
	}

	void D3D12Wrapper::CreateScissorRect(UINT a_windowWidth, UINT a_windowHeight)
	{
		// シザー矩形
		// ビューポートに表示された画像のどこからどこまでを画面に映し出すのかの設定
		m_scissorRect.left = 0;
		m_scissorRect.right = a_windowWidth;
		m_scissorRect.top = 0;
		m_scissorRect.bottom = a_windowHeight;
	}

	void D3D12Wrapper::CreateBackBuffer()
	{
		// バックバッファをスワップチェインから取得
		for (UINT _i = 0; _i < BACKBUFFER_COUNT; ++_i)
		{
			m_backBuffers[_i].Create(m_cpSwapChain.Get(), _i, Resource::TextureUsage::RTV);
		}
	}


	void D3D12Wrapper::CreateCommandContext()
	{
		m_upCommandContext = std::make_unique<CommandContext>();
		m_upCommandContext->Init(m_cpDevice.Get());
	}

	void D3D12Wrapper::CreateFrameManager()
	{
		m_upFrameManager = std::make_unique<FrameManager>();
		m_upFrameManager->Init(m_cpDevice.Get());
	}

	void D3D12Wrapper::CreateAsyncGPUManager()
	{
		m_upAsyncGPUManager = std::make_unique<AsyncGPUManager>();
		m_upAsyncGPUManager->Init();
	}

	void D3D12Wrapper::CloseAndExecuteComdLists(GraphicsCommandList* a_pCmdList)
	{
		// コマンドリストを実行
		m_upCommandContext->RefDirectPool()->ExecuteImmediate(a_pCmdList);
	}

	void D3D12Wrapper::SetBackBuffer()
	{
		// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
		auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
			m_backBuffers[m_currentBackBufferIndex].GetRTV()
		);

		// レンダーターゲットを設定
		m_pCmdList->OMSetRenderTargets(
			1,
			&_cpuHandle,
			FALSE,
			nullptr
		);

		// バッファクリア
		const float _clearColor[] = { 0.0f,0.0f,0.0f,1.0f };
		m_pCmdList->ClearRenderTargetView(_cpuHandle, _clearColor, 0, nullptr);		// レンダーターゲット
	}

	void D3D12Wrapper::SubmitDirectCommandList(GraphicsCommandList* a_pCmdList)
	{
		m_upCommandContext->RefDirectPool()->SubmitList(a_pCmdList);
	}

	void D3D12Wrapper::SubmitCopyCommandList(GraphicsCommandList * a_pCmdList)
	{
		m_upCommandContext->RefCopyPool()->SubmitList(a_pCmdList);
	}

	void D3D12Wrapper::SubmitComputeCommandList(GraphicsCommandList * a_pCmdList)
	{
		m_upCommandContext->RefComputePool()->SubmitList(a_pCmdList);
	}

	void D3D12Wrapper::ExecuteDirectCommandList()
	{
		// 実行待ちリストを処理
		m_upCommandContext->RefDirectPool()->ExecutePendingLists();

		// 終了待機
		m_upFrameManager->WaitForFrame();
	}

	void D3D12Wrapper::ExecuteCopyCommandList()
	{

	}

	void D3D12Wrapper::ExecuteComputeCommandList()
	{
		
	}

	void D3D12Wrapper::ExecuteAsyncCopy(std::function<void(GraphicsCommandList*)> a_recordCmds, std::function<void()> a_onComplete)
	{
		// 非同期マネージャーからアロケーターをもらう
		auto* _allocator = m_upAsyncGPUManager->AcquireAllocator(m_cpDevice.Get(), AsyncCommandType::Copy);

		// コピー用のコマンドプールからリストをもらう (内部で_allocatorを使ってResetされる)
		GraphicsCommandList* _cmdList = m_upCommandContext->RefCopyPool()->AcquireList(m_cpDevice.Get(), _allocator);

		// 外部から渡された「コマンドを積む処理」を実行
		if (a_recordCmds) {
			a_recordCmds(_cmdList);
		}

		// コンテキストにリストを返し、一括実行 (戻り値のフェンス値を受け取る)
		m_upCommandContext->RefCopyPool()->SubmitList(_cmdList);

		// ※注意: もし他にも同時に積みたいパスがあれば、ExecutePendingListsの呼び出しは遅らせてもOKです。
		// 今回は即座に裏スレッドへ投げる想定でここでExecuteします。
		UINT64 _fenceValue = m_upCommandContext->RefCopyPool()->ExecutePendingLists();

		// 非同期マネージャーに監視を依頼する（キュー管理と寿命監視の連携）
		m_upAsyncGPUManager->RegisterTask(
			AsyncCommandType::Copy,
			_allocator,
			m_upCommandContext->RefCopyPool()->GetFence(),
			_fenceValue,
			a_onComplete
		);
	}


	D3D12Wrapper::D3D12Wrapper()
	{}

	D3D12Wrapper::~D3D12Wrapper()
	{}
}