#pragma once

class Device;
class CommandQueue;
class CommandAllocator;
class CommandList;
class Fence;
class SwapChain;
class Viewport;
class ScissorRectangle;

struct BackBuffer
{
	RenderTarget renderTarget;
	Engine::Resource::Handle<RTV> rtvHandle;
};

// フレーム数分必要なリソース
struct D3DFrameResource
{
	// コマンドアロケーター
	std::unique_ptr<CommandAllocator>	upCommandAllocator = nullptr;

	// フレーム中にフェンスが見た最後の値記録
	UINT64								fenceValue =  0;		
};

class D3D12Wrapper
{
public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="a_hWnd">ウィンドウハンドル</param>
	/// <param name="a_windowWidth">ウィンドウ横</param>
	/// <param name="a_windowHeight">ウィンドウ縦</param>
	/// <returns>初期化成功 = true</returns>
	bool Init(
		const HWND& a_hWnd,
		UINT a_windowWidth,
		UINT a_windowHeight
	);

	// 描画に使うオブジェクトとその生成関数群
	bool CreateRenderTarget();			// レンダーターゲットを生成

	/// <summary>
	/// 終了処理
	/// </summary>
	void Shutdown();

	/// <summary>
	/// フレーム開始処理
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// フレーム終了処理
	/// </summary>
	/// <param name="a_isVsync">垂直同期有効化フラグ</param>
	void EndFrame(
		bool a_isVsync = true
	);

	void CommandQueueReset();

	void SetViewportAndRect();

	void WaitRender(UINT a_frameIndex);										// 描画完了を待つ処理
	void WaitRender();										// 描画完了を待つ処理
	void SignalRenderFence();								// フェンスにシグナルを送る処理

	void ResourceBarrier(
		ID3D12Resource* a_pResource,
		D3D12_RESOURCE_STATES a_before,
		D3D12_RESOURCE_STATES a_after
	);

	/// <summary>
	/// レンダーターゲットをクリア
	/// </summary>
	/// <param name="a_renderTargetView">指定レンダーターゲット</param>
	/// <param name="a_colorRGBA">レンダーターゲットのクリアカラー</param>
	/// <param name="a_numRects">矩形数</param>
	/// <param name="a_pRects">矩形ポインタ</param>
	void ClearRenderTargetView(
		D3D12_CPU_DESCRIPTOR_HANDLE a_renderTargetView,
		DirectX::XMFLOAT4 a_colorRGBA = { 0.0f,0.0f,1.0f,1.0f },
		UINT a_numRects = 0,
		const D3D12_RECT* a_pRects = nullptr
	);


	/// <summary>
	/// 深度ステンシルビューをクリア
	/// </summary>
	/// <param name="a_depthStencilView">指定対象のハンドル</param>
	/// <param name="a_clearFlags">クリアフラグ</param>
	/// <param name="a_depth">奥(1.0)</param>
	/// <param name="a_stencil">手前(0.0)</param>
	/// <param name="a_numRects">矩形数</param>
	/// <param name="a_pRects">矩形ポインタ</param>
	void ClearDepthStencilView(
		D3D12_CPU_DESCRIPTOR_HANDLE a_depthStencilView,
		D3D12_CLEAR_FLAGS a_clearFlags = D3D12_CLEAR_FLAG_DEPTH,
		float a_depth = 1.0f,
		float a_stencil = 0.0f,
		UINT a_numRects = 0,
		const D3D12_RECT* a_pRects = nullptr
	);

	void SetBackBuffer();
public:
	// ゲッター
	ID3D12Device* GetDevice();									// デバイス
	ID3D12Device5* GetDevice5();
	ID3D12GraphicsCommandList* GetCommandList();	// コマンドリスト
	ID3D12GraphicsCommandList4* GetCommandList4();	// コマンドリスト
	UINT CurrentBackBufferIndex();									// 現在のフレーム番号
	UINT CurrentCPUFrameIndex();
	IDXGISwapChain* GetSwapChain(); 
	ID3D12Resource* GetCurrentRenderTarget();
	ID3D12CommandQueue* GetCommandQueue();

private:
	// DirectX12のオブジェクト
	std::unique_ptr<Device>				m_upDevice				= nullptr;		// デバイス
	std::unique_ptr<SwapChain>			m_upSwapChain			= nullptr;		// スワップチェイン

	std::unique_ptr<CommandQueue>		m_upCommandQueue		= nullptr;		// コマンドキュー
	std::unique_ptr<CommandList>		m_upCommandList			= nullptr;		// コマンドリスト

	HANDLE								m_fenceEvent			= nullptr;		// フェンスで使うイベント
	std::unique_ptr<Fence>				m_upFence				= nullptr;		// フェンス
	UINT								m_currentFenceValue = 0;

	std::unique_ptr<Viewport>			m_upViewport			= nullptr;		// ビューポート
	std::unique_ptr<ScissorRectangle>	m_upScissorRect			= nullptr;		// シザー矩形

private:


	// バックバッファー
	BackBuffer m_backBuffer[BACKBUFFER_COUNT];

	// フレームリソース
	D3DFrameResource m_frameResource[CPU_FRAME_COUNT] = {};

	// 描画ループで使用するもの
	UINT m_cpuFrameIndex = 0;								// 現在のフレームインデックス
	ID3D12Resource* m_currentRenderTarget = nullptr;		// 現在のフレームのレンダーターゲット
	
private:
	// シングルトン
	// ユニークポインタ使用のため処理はないがcpp側に書いている
	D3D12Wrapper();
	~D3D12Wrapper();
public:
	// インスタンス取得
	static D3D12Wrapper& Instance()
	{
		static D3D12Wrapper _instance;
		return _instance;
	}
};

