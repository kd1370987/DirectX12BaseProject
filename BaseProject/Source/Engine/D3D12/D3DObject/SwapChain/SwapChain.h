#pragma once

class SwapChain
{
public:

	/// <summary>
	/// スワップチェイン生成
	/// </summary>
	/// <param name="a_hWnd">ウィンドウハンドル</param>
	/// <param name="a_frameBufferWidth">横幅</param>
	/// <param name="a_frameBufferHeight">縦</param>
	/// <param name="a_pCmdQueue">コマンドキューポインタ</param>
	/// <returns>成功 = true</returns>
	bool Create(
		IDXGIFactory6* a_pFactory,
		HWND a_hWnd,
		UINT a_frameBufferWidth,
		UINT a_frameBufferHeight,
		ID3D12CommandQueue* a_pCmdQueue
	);

	/// <summary>
	/// プレゼント
	/// </summary>
	/// <param name="a_isVsync">垂直同期フラグ</param>
	void Present(bool a_isVsync);

	UINT GetCurrentBackBufferIndex();

	void GetBuffer(UINT a_idx,ComPtr<ID3D12Resource>& a_renderTarget);

	IDXGISwapChain* Get() { return m_cpSwapChain.Get(); }

	void Update();

private:

	ComPtr<IDXGISwapChain4> m_cpSwapChain = nullptr;
	UINT m_currentBackBufferIndex = 0;

	BOOL m_isAllowTearing = FALSE;
};