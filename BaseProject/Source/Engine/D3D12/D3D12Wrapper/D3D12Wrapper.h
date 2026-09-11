#pragma once

namespace Engine::D3D12
{
	// 前方宣言
	class CommandContext;
	class FrameManager;
	class AsyncGPUManager;
	class DescriptorHeapManager;

	/// <summary>
	/// まとまった単位(モデル1体など)でGPUへ転送するためのバッチ
	///
	/// メッシュ1個ごとにExecuteCommandListsとSignalを打つと、
	/// モデル1体で数十〜数百回のsubmitになってしまう。
	/// Beginで確保したコマンドリストへまとめて積み、Endで1回だけ実行する。
	/// </summary>
	struct AsyncBuildBatch
	{
		ID3D12CommandAllocator* pCopyAllocator = nullptr;
		ID3D12CommandAllocator* pComputeAllocator = nullptr;

		GraphicsCommandList* pCopyCmdList = nullptr;
		GraphicsCommandList* pComputeCmdList = nullptr;

		// 転送完了まで生かしておく中間バッファ
		std::vector<ComPtr<ID3D12Resource>> keepAliveResources;

		bool IsValid() const { return pCopyCmdList != nullptr || pComputeCmdList != nullptr; }
	};

	//==========================================================================================
	// コマンドキュー・フレーム同期・非同期転送
	//
	// デバイスとバックバッファは GraphicsEngine の持ち物になった。
	// ここはデバイスを借りて、キューとフェンスとアロケーターの面倒だけを見る。
	// GetDevice() は呼び出し元が多いので、借り物をそのまま返す形で残してある
	//==========================================================================================
	class D3D12Wrapper
	{
	public:

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="a_pDevice">デバイス(借り物) : 実体は GraphicsEngine が持つ</param>
		void Init(Device* a_pDevice);

		/// <summary>
		/// 終了処理 : デバイスは解放しない(持ち主は GraphicsEngine)
		/// </summary>
		void Release();

		/// <summary>
		/// フレーム開始処理 : このフレームのアロケーターが空くまで待つ
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// フレーム終了処理 : 実行待ちのリストを流して、フレーム終了のシグナルを打つ。
		/// 画面の切り替え(Present)はバックバッファ側で、この後に行う
		/// </summary>
		void EndFrame();

		/// <summary>
		/// ランタイム以外の初期化時などにGPU操作が必要なさいに使う関数
		/// </summary>
		/// <param name="a_pCmdList">コマンドを積んだリスト</param>
		void CloseAndExecuteComdLists(GraphicsCommandList* a_pCmdList);

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
		/// まとまった単位でGPU転送を行うためのバッチを開く
		/// </summary>
		/// <param name="a_useCopy">アップロード転送用のコピーリストを確保するか</param>
		/// <param name="a_useCompute">BLAS構築用のコンピュートリストを確保するか</param>
		AsyncBuildBatch BeginAsyncBuildBatch(bool a_useCopy = true, bool a_useCompute = true);

		/// <summary>
		/// バッチを閉じて実行する
		/// コピー実行 → シグナル → コンピュートキューで待機 → コンピュート実行 の順に流す。
		/// BLASはコピーで転送したメガバッファを直接読むため、この順序でないと未転送のデータを掴む。
		/// </summary>
		/// <param name="a_batch">閉じるバッチ : 呼び出し後は空になる</param>
		/// <param name="a_onComplete">GPU処理完了時に裏で呼ばれるコールバック</param>
		void EndAsyncBuildBatch(AsyncBuildBatch& a_batch, std::function<void()> a_onComplete = nullptr);

		/// <summary>
		/// すべての非同期タスクが完了するまでCPUを待機させる
		/// </summary>
		void WaitForAsyncTasks();

		/// <summary>
		/// すべてのGPU処理が終わるのを待機
		/// 待つのはフレーム終了のシグナルまで。その後に積まれる Present は含まない
		/// </summary>
		void WaitForFrame();

		/// <summary>
		/// 全キュー(描画・コピー・コンピュート)を空にする : 終了処理用。
		/// 新しくシグナルを打ってから待つので、最後の Present も終わっている。
		/// スワップチェインやバックバッファを捨てる前はこちらを通すこと
		/// (WaitForFrame では Present が走っている最中に捨てることになり、デバッグレイヤーが CORRUPTION で止まる)
		/// </summary>
		void WaitForGPUIdle();

	public:
		// ゲッター
		Device* GetDevice();							// デバイス取得(借り物)
		UINT CurrentCPUFrameIndex();					// 現在のフレーム番号

		CommandQueue* GetCommandQueue();			// 描画キュー
		CommandQueue* GetCopyCommandQueue();		// コピーキュー
		CommandQueue* GetComputeCommandQueue();		// コンピュートキュー

		GraphicsCommandList* GetDirectCommandList();	// コマンドリスト取得

		UINT64 GetCurrentFenceValue();		// 最後にシグナルを送った値
		UINT64 GetCompletedFenceValue();	// GPUが実際に完了させた値
		UINT64 GetNextFenceValue();			// 記録中フレームの終わりにシグナルされる値

	private:
		// コマンドコンテキスト
		void CreateCommandContext();

		// フレームマネージャー
		void CreateFrameManager();

		// 非同期マネージャー
		void CreateAsyncGPUManager();
	private:

		// デバイス(借り物)。実体は GraphicsEngine が持っている
		Device* m_pDevice = nullptr;

		// コマンド管理
		std::unique_ptr<CommandContext> m_upCommandContext = nullptr;

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