#pragma once

namespace Engine::Graphics
{
	enum class AsyncCommandType
	{
		Copy,
		Compute
	};

	/// <summary>
	/// まとまった単位(モデル1体など)でGPUへ転送するためのバッチ
	///
	/// メッシュ1個ごとにExecuteCommandListsとSignalを打つと、
	/// モデル1体で数十〜数百回のsubmitになってしまう。
	/// GraphicsEngine::BeginAsyncBuildBatch で確保したコマンドリストへまとめて積み、
	/// EndAsyncBuildBatch で1回だけ実行する。
	/// </summary>
	struct AsyncBuildBatch
	{
		ID3D12CommandAllocator* pCopyAllocator = nullptr;
		ID3D12CommandAllocator* pComputeAllocator = nullptr;

		D3D12::GraphicsCommandList* pCopyCmdList = nullptr;
		D3D12::GraphicsCommandList* pComputeCmdList = nullptr;

		// 転送完了まで生かしておく中間バッファ
		std::vector<ComPtr<ID3D12Resource>> keepAliveResources;

		bool IsValid() const { return pCopyCmdList != nullptr || pComputeCmdList != nullptr; }
	};

	// 実行中の非同期タスクを管理する構造体
	struct AsyncTask
	{
		AsyncCommandType type;
		ComPtr<ID3D12CommandAllocator> cpAllocator; // 使用中のアロケーター
		D3D12::Fence* pTargetFence;                        // 監視するキューのフェンス
		UINT64 targetFenceValue;                    // 目標フェンス値
		std::function<void()> callback;             // 完了時に呼ぶ関数
	};

	class AsyncGPUManager
	{
	public:
		AsyncGPUManager();
		~AsyncGPUManager();

		void Init();
		void Release();

		/// <summary>
		/// 新しい非同期タスク用にアロケーターを取得（なければ作成、あればフリーから再利用）
		/// </summary>
		ID3D12CommandAllocator* AcquireAllocator(D3D12::Device* a_pDevice, AsyncCommandType a_type);

		/// <summary>
		/// コマンド発行後、監視リストにタスクを登録する
		/// </summary>
		void RegisterTask(
			AsyncCommandType a_type,
			ID3D12CommandAllocator* a_pAllocator,
			D3D12::Fence* a_pFence,
			UINT64 a_targetFenceValue,
			std::function<void()> a_onComplete
		);

	private:
		std::mutex m_mutex;

		// アロケーターのフリーリスト（再利用可能になったもの）
		std::vector<ComPtr<ID3D12CommandAllocator>> m_freeCopyAllocators;
		std::vector<ComPtr<ID3D12CommandAllocator>> m_freeComputeAllocators;

		// 実行中のタスク（GPU処理待ち）
		std::vector<AsyncTask> m_inFlightTasks;

		// バックグラウンド監視スレッド
		std::thread m_workerThread;
		std::atomic<bool> m_isExitWorker;

		void WorkerThreadMain();
	};
}