#include "RenderingEngine.h"

#include "../../D3D12/D3DObject/Device/Device.h"
#include "../../D3D12/D3DObject/CommandQueue/CommandQueue.h"
#include "../../D3D12/D3DObject/CommandAllocator/CommandAllocator.h"
#include "../../D3D12/D3DObject/CommandList/CommandList.h"
#include "../../D3D12/D3DObject/Fence/Fence.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3DObject/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "Engine/D3D12/D3DObject/DescriptorHeap/RTVHeap/RTVHeap.h"

#include "Engine/D3D12/D3DObject/SwapChain/SwapChain.h"
#include "Engine/D3D12/D3DObject/Viewport/Viewport.h"
#include "Engine/D3D12/D3DObject/ScissorRectangle/ScissorRectangle.h"

bool RenderingEngine::Init(const HWND& a_hWnd, UINT a_windowWidth, UINT a_windowHeight)
{
	// GPUリソース初期化

	// デバイス作成
	m_upDevice = std::make_unique<Device>();
	if (!m_upDevice->Init()) 
	{
		assert(0 && "デバイス&ファクトリの生成に失敗");
		return false;
	}
	// コマンドキュー作成
	m_upCommandQueue = std::make_unique<CommandQueue>();
	if (!m_upCommandQueue->Create(
			m_upDevice->GetDevice(),
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			D3D12_COMMAND_QUEUE_FLAG_NONE
		)
	)
	{
		assert(0 && "コマンドキューの生成に失敗");
		return false;
	}
	// スワップチェイン作成
	m_upSwapChain = std::make_unique<SwapChain>();
	if (!m_upSwapChain->Create(
		m_upDevice->GetDxgiFactory(),
			a_hWnd, 
			a_windowWidth,
			a_windowHeight,
			m_upCommandQueue->Get()
		)
	)
	{
		assert(0 && "スワップチェインの生成に失敗");
		return false;
	}
	// コマンドアロケーター作成
	m_upCommandAllocator = std::make_unique<CommandAllocator>();
	if (!m_upCommandAllocator->Create(
			m_upDevice->GetDevice(),
			CPU_FRAME_COUNT,
			D3D12_COMMAND_LIST_TYPE_DIRECT
		)
	)
	{
		assert(0 && "コマンドアロケーターの生成に失敗");
		return false;
	}
	// コマンドリスト作成
	m_upCommandList = std::make_unique<CommandList>();
	if (!m_upCommandList->Create(
		m_upDevice->GetDevice(),
		m_upCommandAllocator->Get(m_cpuFrameIndex)
	))
	{
		assert(0 && "コマンドリスト作成失敗");
		return false;
	}

	// フェンス作成
	for (auto _i = 0u; _i < CPU_FRAME_COUNT; ++_i)
	{
		m_fenceValue[_i] = 0;
	}
	m_upFence = std::make_unique<Fence>();
	if (!m_upFence->Create(m_upDevice->GetDevice()))
	{
		assert(0 && "フェンス作成失敗");
		return false;
	}
	m_fenceValue[m_cpuFrameIndex]++;

	// 同期を行うときのイベントハンドラを作成する
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	DescriptorHeapManager::Instance().Init();

	// ビューポートとシザー矩形を生成
	m_upViewport = std::make_unique<Viewport>();
	m_upViewport->Create(a_windowWidth, a_windowHeight);
	m_upScissorRect = std::make_unique<ScissorRectangle>();
	m_upScissorRect->Create(a_windowWidth, a_windowHeight);

	if (!CreateRenderTarget())
	{
		assert(0 && "レンダーターゲットの生成に失敗");
		return false;
	}
	if (!CreateDepthStencil(a_windowWidth, a_windowHeight))
	{
		assert(0 && "デプスステンシルバッファの生成に失敗");
		return false;
	}

	// 初期化成功
	return true;
}

//==================================================================================
// 
// 描画開始・描画終了
// 
//==================================================================================
void RenderingEngine::BeginRender()
{
	// バックバッファ番号更新
	m_upSwapChain->Update();

	m_cpuFrameIndex = (m_cpuFrameIndex + 1) % static_cast<UINT>(CPU_FRAME_COUNT);

	// 次のフレームの描画準備がまだであれば待機する
	WaitRender();

	// 現在のレンダーターゲットを更新
	m_currentRenderTarget = m_pRenderTargets[m_upSwapChain->GetCurrentBackBufferIndex()].Get();

	CommandQueueReset();

	// ビューポートとシザー矩形を設定
	m_upCommandList->SetViewports(1,&m_upViewport->Get());
	m_upCommandList->SetScissorRects(1,&m_upScissorRect->Get());

	// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
	auto _currentRtvHandle = DescriptorHeapManager::Instance().GetDescriptorRTV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();;
	_currentRtvHandle.ptr += m_upSwapChain->GetCurrentBackBufferIndex() * m_rtvDescriptorSize;

	// 深度ステンシルのディスクリプタヒープの開始アドレス取得
	auto _currentDsvHandle = DescriptorHeapManager::Instance().GetDescriptorDSV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();

	// レンダーターゲットが使用可能になるまで待つ
	m_upCommandList->ResourceBarrier(
		m_currentRenderTarget, 
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// レンダーターゲットを設定
	/*m_upCommandList->SetRenderTarget(
		1,
		&_currentRtvHandle, 
		FALSE, 
		&_currentDsvHandle
	);*/

	// バッファクリア
	//m_upCommandList->ClearRenderTargetView(_currentRtvHandle);		// レンダーターゲット
	m_upCommandList->ClearDepthStencilView(_currentDsvHandle);		// 深度ステンシル
}
void RenderingEngine::EndRender(bool a_isVsync)
{
	// レンダーターゲットに書き込みが終わるまで待つ
	m_upCommandList->ResourceBarrier(
		m_currentRenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	// コマンドの記録を終了
	m_upCommandList->Close();

	// コマンドを実行
	ID3D12CommandList* _ppCmdLists[] = { m_upCommandList->NGet() };
	m_upCommandQueue->Get()->ExecuteCommandLists(1, _ppCmdLists);

	SignalRenderFence();

	// スワップチェーンを切替
	m_upSwapChain->Present(a_isVsync);
}

void RenderingEngine::CommandQueueReset()
{
	// コマンドキューを初期化して命令をためる準備をする
	m_upCommandAllocator->Reset(m_cpuFrameIndex);
	m_upCommandList->Reset(m_upCommandAllocator->Get(m_cpuFrameIndex));
}

//==================================================================================
// 
// ゲッター
// 
//==================================================================================
ID3D12Device* RenderingEngine::GetDevice()
{
	// デバイスの取得
	return m_upDevice->GetDevice();
}
ID3D12GraphicsCommandList* RenderingEngine::GetCommandList()
{
	// コマンドリストの取得
	return m_upCommandList->NGet();
}
UINT RenderingEngine::CurrentBackBufferIndex()
{
	// 現在のフレーム番号取得
	return m_upSwapChain->GetCurrentBackBufferIndex();
}

UINT RenderingEngine::CurrentCPUFrameIndex()
{
	return m_cpuFrameIndex;
}

IDXGISwapChain* RenderingEngine::GetSwapChain()
{
	return m_upSwapChain->Get();
}

ID3D12Resource* RenderingEngine::GetCurrentRenderTarget()
{
	return m_pRenderTargets[m_upSwapChain->GetCurrentBackBufferIndex()].Get();
}

ID3D12CommandQueue* RenderingEngine::GetCommandQueue()
{
	return m_upCommandQueue->Get();
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
	for (UINT _i = 0; _i < BACKBUFFER_COUNT; ++_i)
	{
		// スワップチェインから描画するテクスチャリソースを取得
		m_upSwapChain->Get()->GetBuffer(
			_i,
			IID_PPV_ARGS(m_pRenderTargets[_i].ReleaseAndGetAddressOf())
		);
		DescriptorHeapManager::Instance().RegisterRTV(m_pRenderTargets[_i].Get());
	}
	m_rtvDescriptorSize = m_upDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return true;
}
bool RenderingEngine::CreateDepthStencil(UINT a_frameBufferWidth, UINT a_frameBufferHeight)
{
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
	auto _hr = m_upDevice->GetDevice()->CreateCommittedResource(
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
	DescriptorHeapManager::Instance().RegisterDSV(m_pDeptchStencilBuffer.Get());

	return true;
}
void RenderingEngine::WaitRender()
{
	// 次のフレームの描画準備がまだであれば待機する
	if (m_upFence->GetCompletedValue() < m_fenceValue[m_cpuFrameIndex])
	{
		// 完了時にイベントを設定
		if (!m_upFence->SetEventOnCompletion(m_fenceValue[m_cpuFrameIndex], m_fenceEvent))
		{
			assert(0 && "フェンスイベントエラー");
			return;
		}

		// 待機処理
		if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE))
		{
			assert(0 && "待機処理エラー");
			return;
		}
	}
}

void RenderingEngine::SignalRenderFence()
{

	m_currentFenceValue++;
	
	m_upCommandQueue->Get()->Signal(
		m_upFence->GetFence(),
		m_currentFenceValue
	);

	m_fenceValue[m_cpuFrameIndex] = m_currentFenceValue;
}

void RenderingEngine::ResourceBarrier(
	ID3D12Resource* a_pResource,
	D3D12_RESOURCE_STATES a_before,
	D3D12_RESOURCE_STATES a_after
)
{
	// レンダーターゲットに書き込みが終わるまで待つ
	m_upCommandList->ResourceBarrier(
		a_pResource,
		a_before,
		a_after
	);
}

void RenderingEngine::ClearRenderTargetView(
	D3D12_CPU_DESCRIPTOR_HANDLE a_renderTargetView,
	DirectX::XMFLOAT4 a_colorRGBA, 
	UINT a_numRects,
	const D3D12_RECT* a_pRects
)
{
	m_upCommandList->ClearRenderTargetView(
		a_renderTargetView,
		a_colorRGBA,
		a_numRects,
		a_pRects
	);
}

void RenderingEngine::ClearDepthStencilView(
	D3D12_CPU_DESCRIPTOR_HANDLE a_depthStencilView,
	D3D12_CLEAR_FLAGS a_clearFlags,
	float a_depth, 
	float a_stencil,
	UINT a_numRects, 
	const D3D12_RECT* a_pRects
)
{
	m_upCommandList->ClearDepthStencilView(
		a_depthStencilView,
		a_clearFlags,
		a_depth,
		a_stencil,
		a_numRects,
		a_pRects
	);
}

void RenderingEngine::SetBackBuffer()
{
	// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
	auto _currentRtvHandle = DescriptorHeapManager::Instance().GetDescriptorRTV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();;
	_currentRtvHandle.ptr += m_upSwapChain->GetCurrentBackBufferIndex() * m_rtvDescriptorSize;

	// レンダーターゲットを設定
	m_upCommandList->SetRenderTarget(
		1,
		&_currentRtvHandle,
		FALSE,
		nullptr
	);

	// バッファクリア
	m_upCommandList->ClearRenderTargetView(_currentRtvHandle);		// レンダーターゲット
}

RenderingEngine::RenderingEngine()
{
}

RenderingEngine::~RenderingEngine()
{
}
