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
		HWND a_hWnd,
		UINT a_frameBufferWidth,
		UINT a_frameBufferHeight,
		ID3D12CommandQueue* a_pCmdQueue
	);

	void Present(UINT a_f, UINT a_s);

	UINT GetCurrentBackBufferIndex();

	void GetBuffer(UINT a_idx,ComPtr<ID3D12Resource> a_renderTarget);

	IDXGISwapChain3* Get() { return m_cpSwapChain.Get(); }

	void Update();

private:

	ComPtr<IDXGISwapChain3> m_cpSwapChain = nullptr;
	UINT m_currentBackBufferIndex = 0;
};