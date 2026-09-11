#include "BackBuffer.h"


namespace Engine::Graphics
{
	void BackBuffer::Create(
		D3D12::DescriptorHeapManager* a_pHeapManager,
		D3D12::Factory* a_pFactory,
		D3D12::CommandQueue* a_pCmdQueue, 
		HWND a_hWnd,
		UINT a_windowWidth,
		UINT a_windowHeight
	)
	{
		// バッファリング関係
		CreateSwapChain(a_pFactory, a_pCmdQueue, a_hWnd, a_windowWidth, a_windowHeight);	// スワップチェイン作成
		CreateViewPort(a_windowWidth, a_windowHeight);										// 描画用領域設定
		CreateScissorRect(a_windowWidth, a_windowHeight);									// 描画範囲作成

		// バックバッファリソース作成
		for (UINT _i = 0; _i < BACKBUFFER_COUNT; ++_i)
		{
			m_backBuffers[_i].Create(a_pHeapManager, m_cpSwapChain.Get(), _i, Resource::TextureUsage::RTV);
		}
	}
	void BackBuffer::BeginFrame()
	{
		m_currentIndex = m_cpSwapChain->GetCurrentBackBufferIndex();
	}
	void BackBuffer::WriteWite(D3D12::GraphicsCommandList* a_pCmd)
	{
		m_backBuffers[m_currentIndex].Barrier(a_pCmd, D3D12_RESOURCE_STATE_PRESENT);
	}
	void BackBuffer::Present(bool a_isVsync, UINT a_flag)
	{
		m_cpSwapChain->Present(a_isVsync ? 1 : 0, 0);
	}
	void BackBuffer::Release()
	{
		for (auto& _tex : m_backBuffers)
		{
			_tex.Release();
		}
	}
	void BackBuffer::CreateSwapChain(
		D3D12::Factory* a_pFactory, 
		D3D12::CommandQueue* a_pCmdQueue,
		HWND a_hWnd, 
		UINT a_windowWidth, 
		UINT a_windowHeight
	)
	{
		// テレイングチェック
		a_pFactory->CheckFeatureSupport(
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
		ENGINE_ERRLOG(a_pCmdQueue, "コマンドキューの取得に失敗");

		// スワップチェインの作成
		ComPtr<IDXGISwapChain1> _swapChain1;
		HRESULT _hr = a_pFactory->CreateSwapChainForHwnd(
			a_pCmdQueue,
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
		m_currentIndex = m_cpSwapChain->GetCurrentBackBufferIndex();
	}
	void BackBuffer::CreateViewPort(UINT a_windowWidth, UINT a_windowHeight)
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
	void BackBuffer::CreateScissorRect(UINT a_windowWidth, UINT a_windowHeight)
	{
		// シザー矩形
		// ビューポートに表示された画像のどこからどこまでを画面に映し出すのかの設定
		m_scissorRect.left = 0;
		m_scissorRect.right = a_windowWidth;
		m_scissorRect.top = 0;
		m_scissorRect.bottom = a_windowHeight;
	}
}