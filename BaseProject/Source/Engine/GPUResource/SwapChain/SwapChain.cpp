#include "SwapChain.h"

bool SwapChain::Create(HWND a_hWnd, UINT a_frameBufferWidth, UINT a_frameBufferHeight, ID3D12CommandQueue* a_pCmdQueue)
{
	if (!a_pCmdQueue)
	{
		return false;
	}
	IDXGIFactory4* _pFactory = nullptr;
	HRESULT _hr= CreateDXGIFactory1(
		IID_PPV_ARGS(&_pFactory)
	);
	if (FAILED(_hr))
	{
		assert(0 && "DXGIファクトリーの生成に失敗");
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
	_hr = _pFactory->CreateSwapChain(
		a_pCmdQueue,
		&_desc,
		&_pSwapChain
	);
	if (FAILED(_hr))
	{
		_pFactory->Release();
		assert(0 && "スワップチェインの生成に失敗");
		return false;
	}

	// IDXGISwapChain3を取得
	_hr = _pSwapChain->QueryInterface(
		IID_PPV_ARGS(m_cpSwapChain.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		_pFactory->Release();
		_pSwapChain->Release();
		assert(0 && "IDXGISwapChain3の取得に失敗");
		return false;
	}

	// バックバッファ番号を取得
	m_currentBackBufferIndex = m_cpSwapChain->GetCurrentBackBufferIndex();

	// 中間素材開放
	_pFactory->Release();
	_pSwapChain->Release();

	return true;
}

void SwapChain::Present(UINT a_f, UINT a_s)
{
	m_cpSwapChain->Present(a_f, a_s);
}

UINT SwapChain::GetCurrentBackBufferIndex()
{
	//return m_cpSwapChain->GetCurrentBackBufferIndex();
	return m_currentBackBufferIndex;
}

void SwapChain::GetBuffer(UINT a_idx, ComPtr<ID3D12Resource> a_renderTarget)
{
	m_cpSwapChain->GetBuffer(
		a_idx,IID_PPV_ARGS(a_renderTarget.ReleaseAndGetAddressOf())
	);
}

void SwapChain::Update()
{
	m_currentBackBufferIndex = m_cpSwapChain->GetCurrentBackBufferIndex();
}
