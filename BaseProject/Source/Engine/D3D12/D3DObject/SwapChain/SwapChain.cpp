#include "SwapChain.h"
namespace Engine::D3D12
{
	bool SwapChain::Create(IDXGIFactory6* a_pFactory, HWND a_hWnd, UINT a_frameBufferWidth, UINT a_frameBufferHeight, ID3D12CommandQueue* a_pCmdQueue)
	{
		if (!a_pCmdQueue || !a_pFactory)
		{
			return false;
		}

		// テレイングチェック
		a_pFactory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&m_isAllowTearing,
			sizeof(m_isAllowTearing)
		);

		// 仕様書作成
		DXGI_SWAP_CHAIN_DESC1 _desc = {};
		_desc.Width = a_frameBufferWidth;							// 幅
		_desc.Height = a_frameBufferHeight;							// 高さ
		_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;					// ピクセルのフォーマット
		_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;			// バッファの用途（出力）
		_desc.BufferCount = BACKBUFFER_COUNT;						// バッファ数(ダブルバッファリング、トリプルバッファリング)
		_desc.SampleDesc.Count = 1;									// マルチサンプリング（なし）
		_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;			// 切替の方式
		_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		_desc.Scaling = DXGI_SCALING_STRETCH;
		_desc.Flags = m_isAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;			// 可変フレームレート

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
		if (FAILED(_hr))
		{
			assert(0 && "スワップチェインの作成失敗");
			return false;
		}

		// コピー
		_swapChain1.As(&m_cpSwapChain);
		_swapChain1.Reset();

		// バックバッファ番号を取得
		m_currentBackBufferIndex = m_cpSwapChain->GetCurrentBackBufferIndex();

		return true;
	}

	void SwapChain::Release()
	{
		m_cpSwapChain.Reset();
	}

	void SwapChain::Present(bool a_isVsync)
	{
		UINT _sync = a_isVsync ? 1 : 0;
		UINT _flags = (!a_isVsync && m_isAllowTearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;

		m_cpSwapChain->Present(_sync, _flags);
	}

	UINT SwapChain::GetCurrentBackBufferIndex()
	{
		return m_currentBackBufferIndex;
	}

	void SwapChain::GetBuffer(UINT a_idx, ComPtr<ID3D12Resource>& a_renderTarget)
	{
		m_cpSwapChain->GetBuffer(
			a_idx, IID_PPV_ARGS(a_renderTarget.ReleaseAndGetAddressOf())
		);
	}

	void SwapChain::Update()
	{
		m_currentBackBufferIndex = m_cpSwapChain->GetCurrentBackBufferIndex();
	}
}