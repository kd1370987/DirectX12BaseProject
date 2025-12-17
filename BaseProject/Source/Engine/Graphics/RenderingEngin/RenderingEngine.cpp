#include "RenderingEngine.h"

#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/GPUResource/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "Engine/GPUResource/DescriptorHeap/RTVHeap/RTVHeap.h"

#include "Engine/GPUResource/SwapChain/SwapChain.h"
#include "Engine/GPUResource/Viewport/Viewport.h"
#include "Engine/GPUResource/ScissorRectangle/ScissorRectangle.h"

bool RenderingEngine::Init(const HWND& a_hWnd, UINT a_windowWidth, UINT a_windowHeight)
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
	m_upSwapChain = std::make_unique<SwapChain>();
	if (!m_upSwapChain->Create(a_hWnd, a_windowWidth, a_windowHeight, m_commandQueue.Get()))
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
	if (!m_commandList.Create(
			m_device.GetDevice(), 
			m_commandAllocator.GetCCurrentAllocator(m_upSwapChain->GetCurrentBackBufferIndex()),
			m_upSwapChain->GetCurrentBackBufferIndex()
		))
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
	m_fenceValue[m_upSwapChain->GetCurrentBackBufferIndex()]++;

	// 同期を行うときのイベントハンドラを作成する
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// ビューポートとシザー矩形を生成
	m_upViewport = std::make_unique<Viewport>();
	m_upViewport->Create(a_windowWidth, a_windowHeight);
	m_upScissorRect = std::make_unique<ScissorRectangle>();
	m_upScissorRect->Create(a_windowWidth, a_windowHeight);


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
	m_currentRenderTarget = m_pRenderTargets[m_upSwapChain->GetCurrentBackBufferIndex()].Get();

	// コマンドキューを初期化して命令をためる準備をする
	m_commandAllocator.Reset(m_upSwapChain->GetCurrentBackBufferIndex());
	m_commandList.Reset(m_commandAllocator.GetCCurrentAllocator(m_upSwapChain->GetCurrentBackBufferIndex()));

	// ビューポートとシザー矩形を設定
	m_commandList.SetViewports(1,&m_upViewport->Get());
	m_commandList.SetScissorRects(1,&m_upScissorRect->Get());

	// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
	auto _currentRtvHandle = DescriptorHeapManager::Instance().GetDescriptorRTV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();;
	_currentRtvHandle.ptr += m_upSwapChain->GetCurrentBackBufferIndex() * m_rtvDescriptorSize;

	// 深度ステンシルのディスクリプタヒープの開始アドレス取得
	auto _currentDsvHandle = DescriptorHeapManager::Instance().GetDescriptorDSV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();

	// レンダーターゲットが使用可能になるまで待つ
	m_commandList.ResourceBarrier(
		m_currentRenderTarget, 
		D3D12_RESOURCE_STATE_PRESENT, 
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// レンダーターゲットを設定
	m_commandList.SetRenderTarget(
		1,
		&_currentRtvHandle, 
		FALSE, 
		&_currentDsvHandle
	);

	// バッファクリア
	m_commandList.ClearRenderTargetView(_currentRtvHandle);		// レンダーターゲット
	m_commandList.ClearDepthStencilView(_currentDsvHandle);		// 深度ステンシル
}
void RenderingEngine::EndRender()
{
	// レンダーターゲットに書き込みが終わるまで待つ
	m_commandList.ResourceBarrier(
		m_currentRenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	// コマンドの記録を終了
	m_commandList.Close();

	// コマンドを実行
	ID3D12CommandList* _ppCmdLists[] = { m_commandList.NGet() };
	m_commandQueue.Get()->ExecuteCommandLists(1, _ppCmdLists);

	// スワップチェーンを切替
	m_upSwapChain->Present(1,0);

	// 描画完了を待つ
	WaitRender();

	// バックバッファ番号更新
	m_upSwapChain->Update();
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
	return m_commandList.NGet();
}
UINT RenderingEngine::CurrentBackBufferIndex()
{
	// 現在のフレーム番号取得
	return m_upSwapChain->GetCurrentBackBufferIndex();
}

IDXGISwapChain3* RenderingEngine::GetSwapChain()
{
	return m_upSwapChain->Get();
}

ID3D12Resource* RenderingEngine::GetCurrentRenderTarget()
{
	return m_pRenderTargets[m_upSwapChain->GetCurrentBackBufferIndex()].Get();
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
	for (UINT _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		// スワップチェインから描画するテクスチャリソースを取得
		m_upSwapChain->Get()->GetBuffer(
			_i,
			IID_PPV_ARGS(m_pRenderTargets[_i].ReleaseAndGetAddressOf())
		);
		DescriptorHeapManager::Instance().RegisterRTV(m_pRenderTargets[_i].Get());
	}
	m_rtvDescriptorSize = m_device.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
	auto _hr = m_device.GetDevice()->CreateCommittedResource(
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
	// 描画終了待ち
	const UINT64 _fenceValue = m_fenceValue[m_upSwapChain->GetCurrentBackBufferIndex()];
	m_commandQueue.Get()->Signal(m_fence.GetFence(), _fenceValue);
	m_fenceValue[m_upSwapChain->GetCurrentBackBufferIndex()]++;				// インクリメントが待機完了

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

RenderingEngine::RenderingEngine()
{
}

RenderingEngine::~RenderingEngine()
{
}
