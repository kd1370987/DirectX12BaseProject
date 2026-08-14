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

		void PushJob(std::function<void()>&& a_job);

		// アクセサ
		uint32_t GetID() const { return m_workerID; }
		JobQueue& RefQueue() { return m_jobQueue; }
		bool IsRunning() const { return m_isRunning.load(std::memory_order_acquire); }

	private:

		void Run();											// タスク処理

		void Execute(std::function<void()>& a_job);			// ジョブの実行

		bool TrySteal(std::function<void()>& a_outJob);		// ほかスレッドからタスクをとってくる

		void WaitForJob();									// ジョブが来るまで待機


	private:

		uint32_t m_workerID = 0;							// 自身のID
		std::thread m_thread;								// 自身のOSスレッド

		JobQueue m_jobQueue;								// 自身が抱えるタスクキュー
		JobPool m_jobPool;

		JobContext* m_pContext = nullptr;					// ジョブシステム側との共通データ

		std::atomic<bool> m_isRunning = false;				// 処理を終了させるか否か
	};
}
