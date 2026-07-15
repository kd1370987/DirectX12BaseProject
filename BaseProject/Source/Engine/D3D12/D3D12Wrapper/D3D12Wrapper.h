#pragma once

namespace Engine::D3D12
{
	// 前方宣言
	class CommandContext;
	class FrameManager;
	class AsyncGPUManager;

	class D3D12Wrapper
	{
	public:

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="a_hWnd">ウィンドウハンドル</param>
		/// <param name="a_windowWidth">ウィンドウ横</param>
		/// <param name="a_windowHeight">ウィンドウ縦</param>
		void Init(const HWND& a_hWnd, UINT a_windowWidth, UINT a_windowHeight);

		/// <summary>
		/// 終了処理
		/// </summary>
		void Release();

		/// <summary>
		/// フレーム開始処理
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// フレーム終了処理
		/// </summary>
		/// <param name="a_isVsync">垂直同期有効化フラグ</param>
		void EndFrame(bool a_isVsync = true);

		/// <summary>
		/// ランタイム以外の初期化時などにGPU操作が必要なさいに使う関数 
		/// </summary>
		/// <param name="a_pCmdList">コマンドを積んだリスト</param>
		void CloseAndExecuteComdLists(GraphicsCommandList* a_pCmdList);


		void SetBackBuffer();

		/// <summary>
		/// 命令の入ったコマンドリストを実行待ちに入れる
		/// </summary>
		void SubmitDirectCommandList(GraphicsCommandList* a_pCmdList);
		void SubmitCopyCommandList(GraphicsCommandList* a_pCmdList);
		void SubmitComputeCommandList(GraphicsCommandList* a_pCmdList);

		/// <summary>
		/// 一括で実行
		/// </summary>
		void ExecuteDirectCommandList();
		void ExecuteCopyCommandList();
		void ExecuteComputeCommandList();


		// バックバッファ作成
		void CreateBackBuffer();

		// ==========================================================
		// 非同期処理用インターフェース
		// ==========================================================

		/// <summary>
		/// 非同期コピー（バックグラウンドロードなど）を実行します
		/// </summary>
		/// <param name="a_recordCmds">コマンドリストに積む処理（Upload->Defaultへの転送など）</param>
		/// <param name="a_onComplete">GPU転送完了時に裏で呼ばれるコールバック（Uploadヒープの解放など）</param>
		void ExecuteAsyncCopy(
			std::function<void(GraphicsCommandList*)> a_recordCmds,
			std::function<void()> a_onComplete
		);

		/// <summary>
		/// 非同期コンピュートを実行します
		/// </summary>
		void ExecuteAsyncCompute(
			std::function<void(GraphicsCommandList*)> a_recordCmds,
			std::function<void()> a_onComplete
		);

		/// <summary>
		/// すべての非同期タスクが完了するまでCPUを待機させる
		/// </summary>
		void WaitForAsyncTasks();

		/// <summary>
		/// すべてのGPU処理が終わるのを待機
		/// </summary>
		void WaitForFrame();

	public:
		// ゲッター
		Adapter* GetDXGIAdapter();						// GPU取得
		Device* GetDevice();							// デバイス取得
		UINT CurrentBackBufferIndex();					// 現在のバックバッファ番号
		UINT CurrentCPUFrameIndex();					// 現在のフレーム番号
		ID3D12Resource* GetCurrentBackBuffar();			// 現在のバックバッファを取得
		const Resource::Texture& GetCurrentBackBuffarTex() const;			// 現在のバックバッファを取得

		const Viewport& GetViewport() const { return m_viewport; }
		const ScissorRect& GetScissorRect() const { return m_scissorRect; }

		CommandQueue* GetCommandQueue();			// 描画キュー
		CommandQueue* GetCopyCommandQueue();		// コピーキュー
		CommandQueue* GetComputeCommandQueue();		// コンピュートキュー

		GraphicsCommandList* GetDirectCommandList();	// コマンドリスト取得

		UINT64 GetCurrentFenceValue();

	private:
		// ---- D3Dオブジェクト作成 ----
		// デバイス関係
		void CreateDxgiFactory();	// DXGIファクトリ作成
		void FindAdapter();			// GPU検索
		void CreateDevice();		// デバイス作成

		// バッファリング関係
		void CreateSwapChain(HWND a_hWnd, UINT a_windowWidth, UINT a_windowHeight);	// スワップチェイン作成
		void CreateViewPort(UINT a_windowWidth, UINT a_windowHeight);				// 描画用領域設定
		void CreateScissorRect(UINT a_windowWidth, UINT a_windowHeight);			// 描画範囲作成


		// コマンドコンテキスト
		void CreateCommandContext();

		// フレームマネージャー
		void CreateFrameManager();

		// 非同期マネージャー
		void CreateAsyncGPUManager();
	private:

		// デバッグ用
		bool m_isDebag = false;

		// デバイス関係
		ComPtr<Device>						m_cpDevice = nullptr;				// ドライバインスタンス
		ComPtr<Factory>					m_cpFactory = nullptr;				// ファクトリー
		ComPtr<Adapter>					m_cpAdapter = nullptr;				// GPU実体

		bool m_isDynamicResourceSupported = false;								// ダイナミックリソースが使えるかどうか

		// バックバッファー関係
		Resource::Texture					m_backBuffers[BACKBUFFER_COUNT];	// バックバッファ
		ID3D12Resource* m_pCurrentRenderTarget = nullptr;						// 現在のバックバッファ

		ComPtr<SwapChain>				m_cpSwapChain = nullptr;			// スワップチェイン
		BOOL								m_isAllowTearing = FALSE;
		UINT								m_currentBackBufferIndex = 0;		// 現在のバックバッファのインデックス

		Viewport							m_viewport;							// ビューポート
		ScissorRect							m_scissorRect;						// シザー矩形

		// コマンド管理
		std::unique_ptr<CommandContext> m_upCommandContext = nullptr;
		GraphicsCommandList* m_pCmdList = nullptr;						// Beginで取得,Endで返却
		
		// フレーム管理
		std::unique_ptr<FrameManager> m_upFrameManager = nullptr;

		// 非同期マネージャー
		std::unique_ptr<AsyncGPUManager> m_upAsyncGPUManager = nullptr;
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

}