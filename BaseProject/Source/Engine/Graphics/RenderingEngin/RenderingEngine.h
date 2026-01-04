#pragma once

class Device;
class CommandQueue;
class CommandAllocator;
class CommandList;
class Fence;

class RootSignature;
class PipelineState;
class DescriptorHeap;

class ModelResource;
class Mesh;

class ConstantBuffer;

class SwapChain;

class Viewport;
class ScissorRectangle;

class RenderingEngine
{
public:

	// エンジン初期化
	bool Init(const HWND& a_hWnd, UINT a_windowWidth, UINT a_windowHeight);

	// 描画開始・描画終了
	void BeginRender();
	void EndRender();

public:
	// ゲッター
	ID3D12Device6* GetDevice();									// デバイス
	ID3D12GraphicsCommandList* GetCommandList();	// コマンドリスト
	UINT CurrentBackBufferIndex();									// 現在のフレーム番号
	UINT CurrentCPUFrameIndex();
	IDXGISwapChain3* GetSwapChain(); 
	ID3D12Resource* GetCurrentRenderTarget();

private:
	// DirectX12のオブジェクト
	std::unique_ptr<Device>				m_upDevice				= nullptr;		// デバイス
	std::unique_ptr<SwapChain>			m_upSwapChain			= nullptr;		// スワップチェイン

	std::unique_ptr<CommandQueue>		m_upCommandQueue		= nullptr;		// コマンドキュー
	std::unique_ptr<CommandAllocator>	m_upCommandAllocator	= nullptr;		// コマンドアロケーター
	std::unique_ptr<CommandList>		m_upCommandList			= nullptr;		// コマンドリスト

	HANDLE								m_fenceEvent			= nullptr;		// フェンスで使うイベント
	std::unique_ptr<Fence>				m_upFence				= nullptr;		// フェンス
	UINT64								m_fenceValue[CPU_FRAME_COUNT];		// フェンスの数

	std::unique_ptr<Viewport>			m_upViewport			= nullptr;		// ビューポート
	std::unique_ptr<ScissorRectangle>	m_upScissorRect			= nullptr;		// シザー矩形

private:

	// 描画に使うオブジェクトとその生成関数群
	bool CreateRenderTarget();			// レンダーターゲットを生成
	bool CreateDepthStencil(UINT a_frameBufferWidth, UINT a_frameBufferHeight);			// 深度ステンシルバッファを生成

	UINT m_rtvDescriptorSize = 0;						// レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12Resource> m_pRenderTargets[BACKBUFFER_COUNT] = { nullptr };		// レンダーターゲット

	ComPtr<ID3D12Resource> m_pDeptchStencilBuffer = nullptr;	// 深度ステンシルバッファ（こっちは一つ）

	std::shared_ptr<RootSignature> m_spRootSignature = nullptr;		// ルートシグネチャ
	std::shared_ptr<PipelineState> m_spPipeLineState = nullptr;		// パイプラインステート


private:
	// 描画ループで使用するもの
	UINT m_cpuFrameIndex = 0;
	ID3D12Resource* m_currentRenderTarget = nullptr;		// 現在のフレームのレンダーターゲット
	void WaitRender();										// 描画完了を待つ処理
	void SignalRenderFence();								// フェンスにシグナルを送る処理
private:
	// シングルトン
	// ユニークポインタ使用のため処理はないがcpp側に書いている
	RenderingEngine();
	~RenderingEngine();
public:
	// インスタンス取得
	static RenderingEngine& Instance()
	{
		static RenderingEngine _instance;
		return _instance;
	}
};

