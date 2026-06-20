#pragma once

namespace Engine::D3D12
{
	using Microsoft::WRL::ComPtr;

	enum class AsyncCommandType
	{
		Copy,
		Compute
	};

	// 実行中の非同期タスクを管理する構造体
	struct AsyncTask
	{
		AsyncCommandType type;
		ComPtr<ID3D12CommandAllocator> cpAllocator; // 使用中のアロケーター
		Fence* pTargetFence;                        // 監視するキューのフェンス
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
		ID3D12CommandAllocator* AcquireAllocator(Device* a_pDevice, AsyncCommandType a_type);

		/// <summary>
		/// コマンド発行後、監視リストにタスクを登録する
		/// </summary>
		void RegisterTask(
			AsyncCommandType a_type,
			ID3D12CommandAllocator* a_pAllocator,
			Fence* a_pFence,
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