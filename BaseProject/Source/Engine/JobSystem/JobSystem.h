#pragma once

namespace Engine::Thread
{
	class JobWorker;
	struct JobContext;

	class JobSystem
	{
	public:

		JobSystem();
		~JobSystem();


		void Init(uint32_t a_threadCount);
		void Release();

		// タスクの追加
		// すべてのタスクはここで受け付けて、内部で各ワーカーに割り振られる
		void PushJob(std::function<void()>&& a_job);

		// 処理の終了待ち : 全処理が終わるまで待機
		void WaitForAll();

	private:

		uint32_t m_workerCount = 0;

		// 実際に動いているワーカースレッド
		std::vector<std::unique_ptr<JobWorker>> m_jobWorkers = {};
		std::unique_ptr<JobContext> m_upJobContext = nullptr;

		// Jobの割り当て先
		uint32_t m_nextWorker = 0;

		// Job完了通知
		std::condition_variable m_jobFinishedCondition;
		std::mutex m_jobFinishedMutex;

		std::atomic<bool> m_isRunning = false;

	};
}