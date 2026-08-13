#pragma once

#include "../JobContext.h"
#include "../JobQueue/JobQueue.h"

namespace Engine::Thread
{
	class JobWorker
	{
	public:


		void Start(JobContext* a_pJobContext);			// タスクの処理開始
		void Stop();									// タスク処理の終了
		
		void PushJob(std::function<void()>&& a_job);

		// アクセサ
		uint32_t GetID() const { return m_workerID; }
		JobQueue& RefQueue() { return m_jobQueue; }
		bool IsRunning() const { return m_isRunning; }
		bool IsIdle() const;

	private:

		void Run();											// タスク処理

		void Execute(std::function<void()>& a_job);			// ジョブの実行

		bool TrySteal(std::function<void()>& a_outJob);		// ほかスレッドからタスクをとってくる

		void WaitForJob();									// ジョブが来るまで待機


	private:

		uint32_t m_workerID = 0;							// 自身のID
		std::thread m_thread;								// 自身のOSスレッド
		JobQueue m_jobQueue;								// 自身が抱えるタスクキュー

		JobContext* m_pContext;								// ジョブシステム側との共通データ

		std::atomic<bool> m_isWorking = false;				// 現在タスクの処理中かどうか
		std::atomic<bool> m_isRunning = false;				// 処理を終了させるか否か

		std::condition_variable m_condition;				// スレッドで待機するための条件変数
		std::mutex m_mutex;
	};
}