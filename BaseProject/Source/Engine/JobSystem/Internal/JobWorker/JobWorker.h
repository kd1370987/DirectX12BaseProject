#pragma once

#include "../JobContext.h"
#include "../JobQueue/JobQueue.h"
#include "../JobPool/JobPool.h"

namespace Engine::Thread
{
	class JobWorker
	{
	public:


		void Start(JobContext* a_pJobContext, uint32_t a_workerID);	// タスクの処理開始

		// 停止要求 : スレッドの終了は待たない。
		// 停止は「全ワーカーへ要求」->「全ワーカーをJoin」の2段で行うこと。
		// 1つずつ 要求->Join とすると、止めたワーカーのキューに残った仕事を
		// ほかのワーカーが引き取れないまま捨てることになる
		void RequestStop();

		void Join();									// スレッドの終了待ち

		/// <summary>
		/// ジョブの実体を1つ作る : この時点ではまだ実行されない
		/// 依存を張り終えてから PushReadyJob() で流すこと
		/// </summary>
		Job* CreateJob(std::function<void()>&& a_task);

		/// <summary>
		/// 実行可能になったジョブをキューへ積む
		/// 依存待ちのジョブをここへ入れないこと
		/// </summary>
		void PushReadyJob(Job* a_pJob);

		// アクセサ
		uint32_t GetID() const { return m_workerID; }
		JobQueue& RefQueue() { return m_jobQueue; }
		bool IsRunning() const { return m_isRunning.load(std::memory_order_acquire); }

	private:

		void Run();							// タスク処理
		void Execute(Job* a_pJob);			// ジョブの実行
		void FinishJob(Job* a_pJob);		// 完了処理 : 後続の待ち数を減らして、解けたものを流す
		bool TrySteal(Job*& a_pOutJob);		// ほかスレッドからタスクをとってくる
		void WaitForJob();					// ジョブが来るまで待機


	private:

		uint32_t m_workerID = 0;							// 自身のID
		std::thread m_thread;								// 自身のOSスレッド

		JobQueue m_jobQueue;								// 自身が抱えるタスクキュー
		JobPool m_jobPool;									// ジョブの実体 : ローカルキュー

		JobContext* m_pContext = nullptr;					// ジョブシステム側との共通データ

		// 後続ジョブの引き取り先 : FinishJob でのみ使う。
		// このワーカースレッド専用なので、毎回確保せず使い回す
		std::vector<Job*> m_continuationBuffer = {};

		std::atomic<bool> m_isRunning = false;				// 処理を終了させるか否か
	};
}
