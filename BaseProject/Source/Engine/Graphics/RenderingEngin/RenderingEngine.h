#pragma once

class RenderingEngine
{
public:
	// バッファリング数
	enum 
	{
		FRAME_BUFFER_COUNT = 2		// 今回はダブルバッファリング
	};

public:

	// エンジン初期化
	bool Init(HWND a_hWnd, UINT a_windowWidth, UINT a_windowHeight);

	// 描画開始・描画終了
	void BeginRender();
	void EndRender();

public:
	// ゲッター
	ID3D12Device6* Device();						// デバイス
	ID3D12GraphicsCommandList* CommandList();		// コマンドリスト
	UINT CurrentBackBufferIndex();					// 現在のフレーム番号

private:
	// DirectX12初期化に使う
	bool CreateDevice();		// デバイスの生成
	bool CreateCommandQueue();	// コマンドキューの生成
	bool CreateSwapChain();		// スワップチェインの生成
	bool CreateCommandList();	// コマンドリストとコマンドアロケーターを生成
	bool CreateFence();			// フェンスを生成（CPUとGPUの同期処理）
	void CreateViewPort();		// ビューポートを生成
	void CreateScissorRect();	// シザー短径を生成（指定した範囲に描画を行う技術）

private:
	// 描画に使うDirectX12のオブジェクト
	HWND m_hWnd;																		// ウィンドウハンドル
	UINT m_frameBufferWidth = 0;														// バッファのサイズ
	UINT m_frameBufferHeight = 0;														// バッファのサイズ
	UINT m_currentBackBufferIndex = 0;													// 現在のバッファ

	ComPtr<ID3D12Device6> m_pDevice = nullptr;											// GPU（6 = バージョン）
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;										// コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr;										// スワップチェイン
	ComPtr<ID3D12CommandAllocator> m_pAllocator[FRAME_BUFFER_COUNT] = { nullptr };		// コマンドアロケーター
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr;							// コマンドリスト
	HANDLE m_fenceEvent = nullptr;														// フェンスで使うイベント
	ComPtr<ID3D12Fence> m_pFence = nullptr;												// フェンス
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];											// フェンスの数
	D3D12_VIEWPORT m_viewPort;															// ビューポート
	D3D12_RECT m_scissor;																// シザー矩形

private:
	// 描画に使うオブジェクトとその生成関数群
	bool CreateRenderTarget();			// レンダーターゲットを生成
	bool CreateDepthStencil();			// 深度ステンシルバッファを生成

	UINT m_rtvDescriptorSize = 0;						// レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr;	// レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };		// レンダーターゲット

	UINT m_dsvDescriptorSize = 0;								// 深度ステンシルのディスクリプターサイズ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr;			// 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pDeptchStencilBuffer = nullptr;	// 深度ステンシルバッファ（こっちは一つ）

private:
	// 描画ループで使用するもの
	ID3D12Resource* m_currentRenderTarget = nullptr;		// 現在のフレームのレンダーターゲット
	void WaitRender();										// 描画完了を待つ処理

private:
	// シングルトン
	RenderingEngine(){}
	~RenderingEngine(){}
public:
	// インスタンス取得
	static RenderingEngine& Instance()
	{
		static RenderingEngine _instance;
		return _instance;
	}
};

