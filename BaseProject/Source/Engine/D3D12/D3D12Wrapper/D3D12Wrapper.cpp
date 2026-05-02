#include "D3D12Wrapper.h"

#include "../../D3D12/D3DObject/Device/Device.h"
#include "../../D3D12/D3DObject/CommandQueue/CommandQueue.h"
#include "../../D3D12/D3DObject/CommandAllocator/CommandAllocator.h"
#include "../../D3D12/D3DObject/CommandList/CommandList.h"
#include "../../D3D12/D3DObject/Fence/Fence.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/D3D12/D3DObject/SwapChain/SwapChain.h"
#include "Engine/D3D12/D3DObject/Viewport/Viewport.h"
#include "Engine/D3D12/D3DObject/ScissorRectangle/ScissorRectangle.h"
namespace Engine::D3D12
{
	bool D3D12Wrapper::Init(const HWND& a_hWnd, UINT a_windowWidth, UINT a_windowHeight)
	{
		// GPUリソース初期化

		// デバイス作成
		m_upDevice = std::make_unique<Device>();
		if (!m_upDevice->Init(false))
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

		// フレームリソース生成
		for (size_t _i = 0; _i < static_cast<size_t>(CPU_FRAME_COUNT); ++_i)
		{
			// コマンドアロケーター作成
			m_frameResource[_i].upCommandAllocator = std::make_unique<CommandAllocator>();
			m_frameResource[_i].upCommandAllocator->Create(m_upDevice->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);

			// フェンスバリュー設定
			m_frameResource[_i].fenceValue = 0;
		}

		// コマンドリスト作成
		m_upCommandList = std::make_unique<CommandList>();
		if (!m_upCommandList->Create(
			m_upDevice->GetDevice(),
			m_frameResource[m_cpuFrameIndex].upCommandAllocator->Get())
			)
		{
			assert(0 && "コマンドリスト作成失敗");
			return false;
		}

		// フェンス作成
		m_upFence = std::make_unique<Fence>();
		if (!m_upFence->Create(m_upDevice->GetDevice()))
		{
			assert(0 && "フェンス作成失敗");
			return false;
		}
		m_frameResource[m_cpuFrameIndex].fenceValue++;

		// 同期を行うときのイベントハンドラを作成する
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);


		// ビューポートとシザー矩形を生成
		m_upViewport = std::make_unique<Viewport>();
		m_upViewport->Create(a_windowWidth, a_windowHeight);
		m_upScissorRect = std::make_unique<ScissorRectangle>();
		m_upScissorRect->Create(a_windowWidth, a_windowHeight);

		//if (!CreateRenderTarget())
		//{
		//	assert(0 && "レンダーターゲットの生成に失敗");
		//	return false;
		//}

		// 初期化成功
		return true;
	}

	void D3D12Wrapper::Shutdown()
	{
		SignalRenderFence();
		WaitRender(m_cpuFrameIndex);

		for (auto& _bb : m_backBuffer)
		{
			_bb.renderTarget.Reset();
		}

		m_currentRenderTarget = nullptr;

		m_upCommandList.reset();
		for (auto& _res : m_frameResource)
		{
			_res.upCommandAllocator.reset();
		}
		m_upFence.reset();

		m_upSwapChain.reset();
		m_upCommandQueue.reset();
		m_upDevice->Release();
		m_upDevice.reset();
	}

	//==================================================================================
	// 
	// 描画開始・描画終了
	// 
	//==================================================================================
	void D3D12Wrapper::BeginFrame()
	{
		// バックバッファ番号更新
		m_upSwapChain->Update();

		// フレームインデックス更新
		m_cpuFrameIndex = (m_cpuFrameIndex + 1) % static_cast<UINT>(CPU_FRAME_COUNT);

		// 次のフレームの描画準備がまだであれば待機する
		WaitRender(m_cpuFrameIndex);

		// 現在のレンダーターゲットを更新
		m_currentRenderTarget = m_backBuffer[m_upSwapChain->GetCurrentBackBufferIndex()].renderTarget.Ref();

		CommandQueueReset();

		// ビューポートとシザー矩形を設定
		m_upCommandList->SetViewports(1, &m_upViewport->Get());
		m_upCommandList->SetScissorRects(1, &m_upScissorRect->Get());

		// レンダーターゲットが使用可能になるまで待つ
		m_upCommandList->ResourceBarrier(
			m_currentRenderTarget,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
	}
	void D3D12Wrapper::EndFrame(bool a_isVsync)
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

	void D3D12Wrapper::CommandQueueReset()
	{
		// コマンドキューを初期化して命令をためる準備をする
		m_frameResource[m_cpuFrameIndex].upCommandAllocator->Reset();
		m_upCommandList->Reset(m_frameResource[m_cpuFrameIndex].upCommandAllocator->Get());
	}

	void D3D12Wrapper::SetViewportAndRect()
	{
		// ビューポートとシザー矩形を設定
		m_upCommandList->SetViewports(1, &m_upViewport->Get());
		m_upCommandList->SetScissorRects(1, &m_upScissorRect->Get());
	}

	//==================================================================================
	// 
	// ゲッター
	// 
	//==================================================================================
	ID3D12Device* D3D12Wrapper::GetDevice()
	{
		// デバイスの取得
		return m_upDevice->GetDevice();
	}
	ID3D12Device5* D3D12Wrapper::GetDevice5()
	{
		return m_upDevice->GetDevice();
	}
	ID3D12GraphicsCommandList* D3D12Wrapper::GetCommandList()
	{
		// コマンドリストの取得
		return m_upCommandList->NGet();
	}
	ID3D12GraphicsCommandList4* D3D12Wrapper::GetCommandList4()
	{
		return m_upCommandList->Get4();
	}
	CommandList* D3D12Wrapper::GetCmdList()
	{
		return m_upCommandList.get();
	}
	UINT D3D12Wrapper::CurrentBackBufferIndex()
	{
		// 現在のフレーム番号取得
		return m_upSwapChain->GetCurrentBackBufferIndex();
	}

	UINT D3D12Wrapper::CurrentCPUFrameIndex()
	{
		return m_cpuFrameIndex;
	}

	IDXGISwapChain* D3D12Wrapper::GetSwapChain()
	{
		return m_upSwapChain->Get();
	}

	ID3D12Resource* D3D12Wrapper::GetCurrentRenderTarget()
	{
		return m_backBuffer[m_upSwapChain->GetCurrentBackBufferIndex()].renderTarget.Ref();
	}

	ID3D12CommandQueue* D3D12Wrapper::GetCommandQueue()
	{
		return m_upCommandQueue->Get();
	}

	//==================================================================================
	// 
	// 描画に使う関数
	// 
	//==================================================================================
	bool D3D12Wrapper::CreateRenderTarget()
	{
		// レンダーターゲット
		// キャンバスのようなもの。
		// 描画先のバックバッファやテクスチャなどのリソースを指す
		for (UINT _i = 0; _i < BACKBUFFER_COUNT; ++_i)
		{
			// スワップチェインから描画するテクスチャリソースを取得
			m_backBuffer[_i].renderTarget.Create(m_upSwapChain->Get(), _i);
			m_backBuffer[_i].rtvHandle = Engine::D3D12::DescriptorHeapManager::Instance().Allocate<Engine::D3D12::RTV>(m_upDevice->GetDevice(), m_backBuffer[_i].renderTarget.Ref(), nullptr);
		}
		return true;
	}
	void D3D12Wrapper::WaitRender(UINT a_frameIndex)
	{
		// 次のフレームの描画準備がまだであれば待機する
		if (m_upFence->GetCompletedValue() < m_frameResource[a_frameIndex].fenceValue)
		{
			// 完了時にイベントを設定
			if (!m_upFence->SetEventOnCompletion(m_frameResource[a_frameIndex].fenceValue, m_fenceEvent))
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

	void D3D12Wrapper::WaitRender()
	{
		// 次のフレームの描画準備がまだであれば待機する
		if (m_upFence->GetCompletedValue() < m_frameResource[m_cpuFrameIndex].fenceValue)
		{
			// 完了時にイベントを設定
			if (!m_upFence->SetEventOnCompletion(m_frameResource[m_cpuFrameIndex].fenceValue, m_fenceEvent))
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

	void D3D12Wrapper::SignalRenderFence()
	{

		m_currentFenceValue++;

		m_upCommandQueue->Get()->Signal(
			m_upFence->GetFence(),
			m_currentFenceValue
		);

		m_frameResource[m_cpuFrameIndex].fenceValue = m_currentFenceValue;
	}

	void D3D12Wrapper::ResourceBarrier(
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

	void D3D12Wrapper::ClearRenderTargetView(
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

	void D3D12Wrapper::ClearDepthStencilView(
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

	void D3D12Wrapper::SetBackBuffer()
	{
		// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
		auto _cpuHandle = Engine::D3D12::DescriptorHeapManager::Instance().GetCPU(
			m_backBuffer[m_upSwapChain->GetCurrentBackBufferIndex()].rtvHandle
		);

		// レンダーターゲットを設定
		m_upCommandList->SetRenderTarget(
			1,
			&_cpuHandle,
			FALSE,
			nullptr
		);

		// バッファクリア
		m_upCommandList->ClearRenderTargetView(_cpuHandle);		// レンダーターゲット
	}

	D3D12Wrapper::D3D12Wrapper()
	{}

	D3D12Wrapper::~D3D12Wrapper()
	{}
}