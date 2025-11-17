#include "RenderingEngine.h"

bool RenderingEngine::Init(HWND a_hWnd, UINT a_windowWidth, UINT a_windowHeight)
{
	// GPUリソース初期化

	// デバイス作成
	if (!m_device.Init()) 
	{
		return false;
	}
	// コマンドキュー作成
	if (!m_commandQueue.Init(m_device.GetDevice()))
	{
		return false;
	}
	// スワップチェイン作成
	if (!CreateSwapChain(a_hWnd, a_windowWidth, a_windowHeight))
	{
		printf("スワップチェインの生成に失敗");
		return false;
	}
	// コマンドアロケーター作成
	if (!m_commandAllocator.Init(m_device.GetDevice()))
	{
		return false;
	}
	// コマンドリスト作成
	if (!m_commandList.Init(
		m_device.GetDevice(), m_commandAllocator.GetCCurrentAllocator(m_currentBackBufferIndex), m_currentBackBufferIndex))
	{
		return false;
	}

	// フェンス作成
	for (auto _i = 0u; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		m_fenceValue[_i] = 0;
	}
	if (m_fence.Init(m_device.GetDevice()))
	{
		return false;
	}
	m_fenceValue[m_currentBackBufferIndex]++;

	// 同期を行うときのイベントハンドラを作成する
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// ビューポートとシザー矩形を生成
	CreateViewPort(a_windowWidth, a_windowHeight);
	CreateScissorRect(a_windowWidth, a_windowHeight);

	if (!CreateRenderTarget())
	{
		printf("レンダーターゲットの生成に失敗");
		return false;
	}
	if (!CreateDepthStencil(a_windowWidth, a_windowHeight))
	{
		printf("デプスステンシルバッファの生成に失敗");
		return false;
	}

	// 初期化成功
	printf("描画エンジンの初期化に成功\n");

	// サイズ記憶
	m_backBufferWidth = a_windowWidth;
	m_backBufferHeight = a_windowHeight;

	return true;
}

//==================================================================================
// 
// 描画開始・描画終了
// 
//==================================================================================
void RenderingEngine::BeginRender()
{
	// 現在のレンダーターゲットを更新
	m_currentRenderTarget = m_pRenderTargets[m_currentBackBufferIndex].Get();

	// コマンドキューを初期化して命令をためる準備をする
	m_commandAllocator.Reset(m_currentBackBufferIndex);
	m_commandList.Reset(m_commandAllocator.GetCCurrentAllocator(m_currentBackBufferIndex));

	// ビューポートとシザー矩形を設定
	m_commandList.GetCommandList()->RSSetViewports(1, &m_viewPort);
	m_commandList.GetCommandList()->RSSetScissorRects(1,&m_scissor);

	// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
	auto _currentRtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	_currentRtvHandle.ptr += m_currentBackBufferIndex * m_rtvDescriptorSize;

	// 深度ステンシルのディスクリプタヒープの開始アドレス取得
	auto _currentDsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

	// レンダーターゲットが使用可能になるまで待つ
	auto _burrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_currentRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_commandList.GetCommandList()->ResourceBarrier(1, &_burrier);

	// レンダーターゲットを設定
	m_commandList.GetCommandList()->OMSetRenderTargets(1, &_currentRtvHandle, FALSE, &_currentDsvHandle);

	// レンダーターゲットをクリア
	const float _clearColor[] = { 0.25f,0.25f,1.0f,1.0f };									// バックバッファの色
	m_commandList.GetCommandList()->ClearRenderTargetView(_currentRtvHandle, _clearColor, 0, nullptr);

	// 深度ステンシルビューをクリア
	m_commandList.GetCommandList()->ClearDepthStencilView(_currentDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}
void RenderingEngine::EndRender()
{
	// レンダーターゲットに書き込みが終わるまで待つ
	auto _barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_currentRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	);
	m_commandList.GetCommandList()->ResourceBarrier(1, &_barrier);

	// コマンドの記録を終了
	m_commandList.GetCommandList()->Close();

	// コマンドを実行
	ID3D12CommandList* _ppCmdLists[] = { m_commandList.GetCommandList() };
	m_commandQueue.Get()->ExecuteCommandLists(1, _ppCmdLists);

	// スワップチェーンを切替
	m_pSwapChain->Present(1, 0);

	// 描画完了を待つ
	WaitRender();

	// バックバッファ番号更新
	m_currentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();
}



//==================================================================================
// 
// ゲッター
// 
//==================================================================================
ID3D12Device6* RenderingEngine::GetDevice()
{
	// デバイスの取得
	return m_device.GetDevice();
}
ID3D12GraphicsCommandList* RenderingEngine::GetCommandList()
{
	// コマンドリストの取得
	return m_commandList.GetCommandList();
}
UINT RenderingEngine::CurrentBackBufferIndex()
{
	// 現在のフレーム番号取得
	return m_currentBackBufferIndex;
}

//==================================================================================
// 
// DirectX12初期化に使う
// 
//==================================================================================
bool RenderingEngine::CreateSwapChain(HWND a_hWnd, UINT a_frameBufferWidth, UINT a_frameBufferHeight)
{
	// スワップチェイン
	// 描画先を複数用意して、順番に描画をしていくことで、ちらつきなどを抑える

	// DXGIファクトリーの生成
	IDXGIFactory4* _pFactory = nullptr;
	HRESULT _hr = CreateDXGIFactory1(IID_PPV_ARGS(&_pFactory));
	if (FAILED(_hr))
	{
		printf("DXGIファクトリーの生成に失敗");
		return false;
	}

	// 仕様書作成
	DXGI_SWAP_CHAIN_DESC _desc = {};
	_desc.BufferDesc.Width = a_frameBufferWidth;									// 幅
	_desc.BufferDesc.Height = a_frameBufferHeight;									// 高さ
	_desc.BufferDesc.RefreshRate.Numerator = 60;									// 何回(60 / 1 = 60)
	_desc.BufferDesc.RefreshRate.Denominator = 1;									// 何秒間の間に(60 / 1 = 1)
	_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;		// 映像のスキャン順序
	_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;						// スケーリング方法
	_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;							// ピクセルのフォーマット
	_desc.SampleDesc.Count = 1;														// マルチサンプリング（なし）
	_desc.SampleDesc.Quality = 0;													// アンチエイリアス設定
	_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;							// バッファの用途（出力）
	_desc.BufferCount = FRAME_BUFFER_COUNT;											// バッファ数
	_desc.OutputWindow = a_hWnd;													// 出力先
	_desc.Windowed = TRUE;															// ウィンドウモード指定
	_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;								// 切替の方式
	_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;							// フルスクリーン切替許可

	// スワップチェインの生成
	IDXGISwapChain* _pSwapChain = nullptr;
	_hr = _pFactory->CreateSwapChain(m_commandQueue.Get(), &_desc, &_pSwapChain);
	if (FAILED(_hr))
	{
		_pFactory->Release();
		printf("スワップチェインの生成に失敗");
		return false;
	}

	// IDXGISwapChain3を取得
	_hr = _pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.ReleaseAndGetAddressOf()));
	if (FAILED(_hr))
	{
		_pFactory->Release();
		_pSwapChain->Release();
		printf("IDXGISwapChain3の取得に失敗");
		return false;
	}

	// バックバッファ番号を取得
	m_currentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 生成に使った中間素材を解放
	_pFactory->Release();
	_pSwapChain->Release();

	return true;
}
void RenderingEngine::CreateViewPort(UINT a_frameBufferWidth, UINT a_frameBufferHeight)
{
	// ビューポート
	// ウィンドウに対してレンダリング結果をどう表示するかの設定
	
	// 左上座標
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;

	// 幅・高さ
	m_viewPort.Width = static_cast<float>(a_frameBufferWidth);
	m_viewPort.Height = static_cast<float>(a_frameBufferHeight);

	// 深度のマッピング範囲（奥行情報・Zバッファの値）
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

}
void RenderingEngine::CreateScissorRect(UINT a_frameBufferWidth, UINT a_frameBufferHeight)
{
	// シザー矩形
	// ビューポートに表示された画像のどこからどこまでを画面に映し出すのかの設定

	m_scissor.left = 0;
	m_scissor.right = a_frameBufferWidth;
	m_scissor.top = 0;
	m_scissor.bottom = a_frameBufferHeight;
}

//==================================================================================
// 
// 描画に使う関数
// 
//==================================================================================
bool RenderingEngine::CreateRenderTarget()
{
	// レンダーターゲット
	// キャンバスのようなもの。
	// 描画先のバックバッファやテクスチャなどのリソースを指す

	// RTV用のディスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC _desc = {};					// ビューをまとめる領域を作成
	_desc.NumDescriptors = FRAME_BUFFER_COUNT;				// 作るディスクリプタの数(RTV,通常フレームバッファの数)
	_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;			// RenderTargetView用のヒープをしてい
	_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;			// RTVなのでNone（シェーダーから参照しないため）

	// ディスクリプタヒープをGPU上に作成
	auto _hr = m_device.GetDevice()->CreateDescriptorHeap(&_desc,IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(_hr))
	{
		printf("RTV用のディスクリプタヒープの作成に失敗");
		return false;
	}

	// ディスクリプタのサイズを取得（隣のディスクリプに進むために一個当たりのサイズを記録）
	m_rtvDescriptorSize = m_device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE _rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

	// 各フレームバッファにRTVを作成
	for (UINT _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		// スワップチェーンを取得（実際に描画するテクスチャリソース）
		m_pSwapChain->GetBuffer(_i,IID_PPV_ARGS(m_pRenderTargets[_i].ReleaseAndGetAddressOf()));
		// バックバッファを描画ターゲットとして使うためのビューを作成
		m_device.GetDevice()->CreateRenderTargetView(m_pRenderTargets[_i].Get(),nullptr,_rtvHandle);
		// 次のRTV用にハンドルをずらす
		_rtvHandle.ptr += m_rtvDescriptorSize;
	}
	return true;
}
bool RenderingEngine::CreateDepthStencil(UINT a_frameBufferWidth, UINT a_frameBufferHeight)
{
	// 深度ステンシルバッファの生成
	// レンダーターゲット内にあるポリゴンがどっちが手前でどっちが奥かを判別する物
	// カメラから見たZの値を持っておくためのバッファ

	// DSV用のディスクリプタヒープを作成する
	D3D12_DESCRIPTOR_HEAP_DESC _heapDesc = {};
	_heapDesc.NumDescriptors = 1;															// ビューの数
	_heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;										// DSV専用ディスクリプタヒープ
	_heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;										// シェーダーから参照しない
	auto _hr = m_device.GetDevice()->CreateDescriptorHeap(&_heapDesc, IID_PPV_ARGS(&m_pDsvHeap));
	if (FAILED(_hr))
	{
		printf("DSV用のディスクリプタヒープ生成失敗");
		return false;
	}

	// ディスクリプタのサイズを取得
	m_dsvDescriptorSize = m_device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// 初期値を作成
	D3D12_CLEAR_VALUE _dsvClearValue;
	_dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;			// 深度を32bitで表現
	_dsvClearValue.DepthStencil.Depth = 1.0f;				// カメラから最も遠い（初期値 = 1.0f）
	_dsvClearValue.DepthStencil.Stencil = 0;				// ステンシル値の初期化

	// 深度ステンシル用リソース（テクスチャ）を作成
	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC _resourceDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,														// 2Dテクスチャとして使う
		0,
		a_frameBufferWidth,																		// 画面サイズに合わせる
		a_frameBufferHeight,																	// 画面サイズに合わせる
		1,
		1,
		DXGI_FORMAT_D32_FLOAT,																	// 深度値は32bit float
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE		// 用途指定 | 参照禁止
	);

	// リソースを実際に作成(GPU上に)
	_hr = m_device.GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,									// 深度書き込み可として作成
		&_dsvClearValue,													// 初期クリア値を指定
		IID_PPV_ARGS(m_pDeptchStencilBuffer.ReleaseAndGetAddressOf())		// 実際のＺバッファを保持するメンバ
	);
	if (FAILED(_hr))
	{
		return false;
	}

	// ディスクリプタを作成
	// 作成したリソースに対するビューを生成（使うための窓口）
	D3D12_CPU_DESCRIPTOR_HANDLE _dsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_device.GetDevice()->CreateDepthStencilView(m_pDeptchStencilBuffer.Get(), nullptr, _dsvHandle);

	return true;
}
void RenderingEngine::WaitRender()
{
	// 描画終了待ち
	const UINT64 _fenceValue = m_fenceValue[m_currentBackBufferIndex];
	m_commandQueue.Get()->Signal(m_fence.GetFence(), _fenceValue);
	m_fenceValue[m_currentBackBufferIndex]++;				// インクリメントが待機完了

	// 次のフレームの描画準備がまだであれば待機する
	if (m_fence.GetCompletedValue() < _fenceValue)
	{
		// 完了時にイベントを設定
		if (m_fence.SetEventOnCompletion(_fenceValue, m_fenceEvent))
		{
			return;
		}

		// 待機処理
		if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE))
		{
			return;
		}
	}
}
