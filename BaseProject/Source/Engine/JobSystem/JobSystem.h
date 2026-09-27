#pragma once

namespace Engine::Thread
{
	class JobWorker;
	struct JobContext;
	class ThreadProfiler;

	class JobSystem
	{
	public:

		JobSystem();
		~JobSystem();


		void Init(uint32_t a_threadCount);
		void Release();

		// タスクの追加
		// すべてのタスクはここで受け付けて、内部で各ワーカーに割り振られる
		Job* PushJob(std::function<void()>&& a_job);

		/// <summary>
		/// 先行ジョブの完了を待ってから走るタスクを追加する
		///
		/// 渡したジョブがすべて終わった時点で自動的にキューへ流れる。
		/// 登録時にすでに終わっていた先行ジョブは、その場で解決済みとして扱う。
		///
		/// 返るポインタはワーカーのジョブプールを指す。
		/// プールはリングで使い回されるため、後続を張る目的以外で長く持たないこと
		/// </summary>
		/// <param name="a_dependencies">先行ジョブ : nullptr は無視される</param>
		Job* PushJob(std::function<void()>&& a_job, std::span<Job* const> a_dependencies);
		Job* PushJob(std::function<void()>&& a_job, std::initializer_list<Job*> a_dependencies);

		/// <summary>
		/// フレーム内で完了を待つ短いジョブを積む(ECS のシステムなど)
		///
		/// PushJob と同じだが、WaitFor で待っているスレッドが代わりに回してよい印が付く。
		/// ロードのように長い処理をここで積まないこと : 待ち側が拾うと長く戻れなくなる
		/// </summary>
		Job* PushFrameJob(std::function<void()>&& a_job, std::span<Job* const> a_dependencies);

		// 処理の終了待ち : 全処理が終わるまで待機
		//
		// 「システムに積まれた全ジョブ」が対象なので、
		// 非同期ロードなど別系統のジョブが走っている間は返ってこない。
		// フレーム内の同期には WaitFor() を使うこと
		void WaitForAll();

		/// <summary>
		/// 指定したジョブ1件が終わるまで待機する
		///
		/// 依存を1点にまとめたフェンスジョブを待つ用途を想定している。
		/// 待っている間は、PushFrameJob で積まれた実行可能なジョブを代わりに回す。
		/// 回せるものが無いときは、どれかのジョブが終わるまで眠る。
		/// 呼ぶ側は待ちに入る前に積めるものを積みきっておくこと
		/// </summary>
		/// <param name="a_pJob">待つジョブ : nullptr なら即座に返る</param>
		void WaitFor(Job* a_pJob);

		// アクセサ
		// 起動しているワーカー数 : 処理をチャンクに分ける粒度を決めるのに使う
		uint32_t GetWorkerCount() const { return m_workerCount; }
		bool IsRunning() const { return m_isRunning.load(std::memory_order_acquire); }

		// スレッドごとの稼働時間の計測 : 起動していないときは nullptr
		// フレームの区切り(EndFrame)と結果の読み出しはメインスレッドから行うこと
		ThreadProfiler* RefThreadProfiler() { return m_upThreadProfiler.get(); }

	private:

		// 積む処理の本体 : a_isHelpable は Job::isHelpable へそのまま入る
		Job* PushJobImpl(std::function<void()>&& a_job, std::span<Job* const> a_dependencies, bool a_isHelpable);

		// 待っている間に回せるジョブ(Job::isHelpable)を、どれかのワーカーのキューから取る
		bool TryTakeHelpableJob(Job*& a_pOutJob);

		// 取ったジョブを呼んだスレッドで回す : 完了の後始末はワーカーと同じ
		void ExecuteOnCaller(Job* a_pJob);

	private:

		uint32_t m_workerCount = 0;

		// スレッドごとの稼働時間の計測 : ワーカーが記録を書き込むので、
		// ワーカーより先に宣言して「ワーカーが片付いた後に壊れる」ようにしておく
		std::unique_ptr<ThreadProfiler> m_upThreadProfiler = nullptr;

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
