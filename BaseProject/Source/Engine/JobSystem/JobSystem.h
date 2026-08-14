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
		Handle<Job> Schedule(std::function<void()>&& a_job);

		// 処理の終了待ち : 全処理が終わるまで待機
		void WaitForAll();

		// アクセサ
		// 起動しているワーカー数 : 処理をチャンクに分ける粒度を決めるのに使う
		uint32_t GetWorkerCount() const { return m_workerCount; }
		bool IsRunning() const { return m_isRunning.load(std::memory_order_acquire); }

	private:

		uint32_t m_workerCount = 0;

		// ワーカーとの共有データ : 完了待ちのカウンタもここが持つ。
		// ワーカーが動いている間ずっと参照されるので、
		// ワーカーより先に宣言して「ワーカーが片付いた後に壊れる」ようにしておく
		std::unique_ptr<JobContext> m_upJobContext = nullptr;

		// 実際に動いているワーカースレッド
		std::vector<std::unique_ptr<JobWorker>> m_jobWorkers = {};

		// Jobの割り当て先
		// ジョブの中からジョブを積む(モデル -> メッシュ/テクスチャ)経路があるため、
		// メインスレッド以外からも進められる。必ずアトミックに回すこと
		std::atomic<uint32_t> m_nextWorker = 0;

		std::atomic<bool> m_isRunning = false;

		

	};
}
