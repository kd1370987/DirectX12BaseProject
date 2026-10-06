#include "JobSystem.h"

#include "Internal/JobContext.h"
#include "Internal/JobWorker/JobWorker.h"
#include "Profile/ThreadProfiler.h"

namespace Engine::Thread
{
	JobSystem::JobSystem()
	{}
	JobSystem::~JobSystem()
	{
		// Release() を呼び忘れても、スレッドが動いたまま共有データが壊れないように畳む。
		// 起動中の std::thread をそのまま破棄すると std::terminate になる
		Release();
	}
	void Engine::Thread::JobSystem::Init(uint32_t a_threadCount)
	{
		if (m_isRunning.load(std::memory_order_acquire))
		{
			ENGINE_WARNING("[JobSystem] すでに初期化されています");
			return;
		}

		if (a_threadCount == 0)
		{
			ENGINE_WARNING("[JobSystem] スレッド数0では初期化できません");
			return;
		}

		ENGINE_LOG("[Init] JobSystemがスレッド数 : %d で初期化されました",(int)a_threadCount);

		m_workerCount = a_threadCount;
		m_nextWorker.store(0, std::memory_order_relaxed);

		// 稼働時間の計測 : Init はメインスレッドから呼ばれるので、呼んだスレッドをメインとして登録する
		m_upThreadProfiler = std::make_unique<ThreadProfiler>();
		m_upThreadProfiler->Init(a_threadCount);
		m_upThreadProfiler->BindMainThread();

		// コンテキスト作成
		m_upJobContext = std::make_unique<JobContext>();
		m_upJobContext->pThreadProfiler = m_upThreadProfiler.get();

		// ワーカースレッドクラスの作成
		m_jobWorkers.resize(a_threadCount);
		m_upJobContext->pJobWorker.resize(a_threadCount);
		for (uint32_t _i = 0; _i < a_threadCount; ++_i)
		{
			m_jobWorkers[_i] = std::make_unique<JobWorker>();
			m_upJobContext->pJobWorker[_i] = m_jobWorkers[_i].get();
		}

		// 参照先がすべて揃ってから起動する。
		// 先に走らせると、起動直後のワークスティールが未設定のポインタを踏む
		m_isRunning.store(true, std::memory_order_release);

		// すべてのスレッドを起動
		for (uint32_t _i = 0; _i < a_threadCount; ++_i)
		{
			m_jobWorkers[_i]->Start(m_upJobContext.get(), _i);
		}
	}

	void Engine::Thread::JobSystem::Release()
	{
		// デストラクタからも呼ばれるので、二重に畳まないようにする
		if (!m_isRunning.load(std::memory_order_acquire)) return;

		// 残っている仕事を片付けてから止める
		WaitForAll();

		m_isRunning.store(false, std::memory_order_release);

		// 停止は「全ワーカーへ要求」->「全ワーカーをJoin」の2段で行う。
		// 1つずつ 要求->Join とすると、先に止めたワーカーのキューに残った仕事を
		// ほかのワーカーが引き取れないまま捨てることになる
		for (auto& _worker : m_jobWorkers)
		{
			_worker->ReserveStop();
		}
		for (auto& _worker : m_jobWorkers)
		{
			_worker->Join();
		}

		// ワーカーを先に片付けてからコンテキストを捨てる。
		// 逆にすると、走っているスレッドが破棄済みの共有データを触る
		m_jobWorkers.clear();
		m_upJobContext.reset();
		m_workerCount = 0;

		// 記録を捨てる前に、メインスレッドの登録を外す (Release はメインスレッドから呼ばれる)
		ThreadProfiler::UnbindCurrentThread();
		m_upThreadProfiler.reset();
	}

	Job* Engine::Thread::JobSystem::PushJob(std::function<void()>&& a_job)
	{
		return PushJob(std::move(a_job), std::span<Job* const>());
	}

	Job* JobSystem::PushJob(std::function<void()>&& a_job, std::initializer_list<Job*> a_dependencies)
	{
		return PushJob(std::move(a_job), std::span<Job* const>(a_dependencies.begin(), a_dependencies.size()));
	}

	Job* JobSystem::PushJob(std::function<void()>&& a_job, std::span<Job* const> a_dependencies)
	{
		return PushJobImpl(std::move(a_job), a_dependencies, false);
	}

	Job* JobSystem::PushFrameJob(std::function<void()>&& a_job, std::span<Job* const> a_dependencies)
	{
		return PushJobImpl(std::move(a_job), a_dependencies, true);
	}

	Job* JobSystem::PushJobImpl(std::function<void()>&& a_job, std::span<Job* const> a_dependencies, bool a_isHelpable)
	{
		// 停止中や未初期化で積むと、カウンタだけ増えて誰も処理しない。
		// WaitForAll() が返らなくなるので、カウンタを触る前に弾く
		if (!m_isRunning.load(std::memory_order_acquire) || m_jobWorkers.empty())
		{
			ENGINE_WARNING("[JobSystem] 停止中または未初期化のためジョブを破棄しました");
			return nullptr;
		}

		// 受付の時点で完了待ちに数える。
		// 依存待ちの間もこのジョブは「まだ終わっていない」ので、
		// ここで数えておかないと WaitForAll() が素通りする
		m_upJobContext->AddPendingJob();

		// 割り当て先の決定。
		// ジョブの中からジョブを積むとここが複数スレッドから同時に走るため、
		// 「加算」と「読み出し」を分けると割り込まれて範囲外の添え字を引く。
		// fetch_add で取った値をそのまま自分の取り分として使う
		const uint32_t _workerIndex =
			m_nextWorker.fetch_add(1, std::memory_order_relaxed)
			% static_cast<uint32_t>(m_jobWorkers.size());

		auto& _upWorker = m_jobWorkers[_workerIndex];

		Job* _pJob = _upWorker->CreateJob(std::move(a_job));

		// キューへ積む(ほかのスレッドから見える)前に決める
		_pJob->isHelpable = a_isHelpable;

		//--------------------------------------------------------------------------------------
		// 依存の登録
		//--------------------------------------------------------------------------------------

		// 有効な先行ジョブの数を数える
		uint32_t _validCount = 0;
		for (Job* _pDependency : a_dependencies)
		{
			if (_pDependency != nullptr) ++_validCount;
		}

		// 依存がなければそのまま流す
		if (_validCount == 0)
		{
			_upWorker->PushReadyJob(_pJob);
			return _pJob;
		}

		// 待ち数は「登録を始める前」に立てきる。
		// 途中で立てると、先に終わった先行ジョブの減算を取りこぼす
		_pJob->waitingCount.store(_validCount, std::memory_order_release);

		// 登録した時点ですでに終わっていたものは、通知が飛んでこないので自分で数える
		uint32_t _alreadyFinishedCount = 0;
		for (Job* _pDependency : a_dependencies)
		{
			if (_pDependency == nullptr) continue;
			if (!_pDependency->AddContinuation(_pJob)) ++_alreadyFinishedCount;
		}

		// 解決済みの分をまとめて引く。
		// 待ち数を0にしたのが自分だったときだけキューへ積む。
		// 先行ジョブ側も同じ判定をしているので、両方が積む二重投入は起きない
		if (_alreadyFinishedCount > 0 &&
			_pJob->waitingCount.fetch_sub(_alreadyFinishedCount, std::memory_order_acq_rel) == _alreadyFinishedCount)
		{
			_upWorker->PushReadyJob(_pJob);
		}

		return _pJob;
	}

	void Engine::Thread::JobSystem::WaitForAll()
	{
		if (!m_upJobContext) return;

		// 待っている間は「動いていない」: ジョブ待ちとして数える
		ThreadStateScope _waitScope(EThreadState::JobWait);
		m_upJobContext->WaitForAllJobs();
	}

	void JobSystem::WaitFor(Job* a_pJob)
	{
		if (a_pJob == nullptr || !m_upJobContext) return;

		JobContext& _context = *m_upJobContext;

		// 待ち人数は「完了印を見る前」に増やし、待ちの間ずっと立てておく。
		// 手伝いの合間に終わったジョブの通知(進み具合)を取りこぼさないため
		_context.jobWaiterCount.fetch_add(1, std::memory_order_seq_cst);

		while (!a_pJob->IsFinished())
		{
			// 探す前に進み具合を控える。探した後に後続として積まれたものは、
			// その先行ジョブの完了で進み具合が変わるので、眠らずに拾い直せる
			const uint64_t _seenEpoch = _context.LoadJobFinishedEpoch();

			// 回せるものがあれば、眠らずに代わりに回す
			Job* _pHelpJob = nullptr;
			if (TryTakeHelpableJob(_pHelpJob))
			{
				ExecuteOnCaller(_pHelpJob);
				continue;
			}

			// 待っている間は「動いていない」: ジョブ待ちとして数える。
			// ワーカーのジョブの中から待った場合も、そのワーカーの Busy から外れる
			ThreadStateScope _waitScope(EThreadState::JobWait);
			_context.WaitForJobProgress(_seenEpoch, [a_pJob]() { return a_pJob->IsFinished(); });
		}

		_context.jobWaiterCount.fetch_sub(1, std::memory_order_seq_cst);
	}

	bool JobSystem::TryTakeHelpableJob(Job*& a_pOutJob)
	{
		for (auto& _upWorker : m_jobWorkers)
		{
			if (_upWorker->RefQueue().TryStealHelpable(a_pOutJob)) return true;
		}
		return false;
	}

	void JobSystem::ExecuteOnCaller(Job* a_pJob)
	{
		JobContext& _context = *m_upJobContext;

		// JobWorker::Execute と同じ流れ。ワーカーのキューから取ったので盗みとしても数える
		ThreadStateScope _busyScope(EThreadState::Busy);
		ThreadProfiler::CountJob();
		ThreadProfiler::CountSteal();

		_context.OnJobDequeued();

		try
		{
			if (a_pJob->task) a_pJob->task();
		}
		catch (const std::exception& _e)
		{
			ENGINE_WARNING("[JobSystem] ジョブが例外で終了しました : %s", _e.what());
		}
		catch (...)
		{
			ENGINE_WARNING("[JobSystem] ジョブが不明な例外で終了しました");
		}

		// 後続を流す : 呼んだスレッドはワーカーではないので、割り当ては PushJob と同じく順番に回す。
		// 受け取り先はスレッドごとに使い回す(待ちの中から入れ子で呼ばれることは無い)
		thread_local std::vector<Job*> t_continuationBuffer = {};
		a_pJob->FinishAndTakeContinuations(t_continuationBuffer);

		for (Job* _pNext : t_continuationBuffer)
		{
			if (_pNext == nullptr) continue;

			// 待ち数を0にしたスレッドだけがキューへ積む(JobWorker::FinishJob と同じ)
			if (_pNext->waitingCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				const uint32_t _workerIndex =
					m_nextWorker.fetch_add(1, std::memory_order_relaxed)
					% static_cast<uint32_t>(m_jobWorkers.size());
				m_jobWorkers[_workerIndex]->PushReadyJob(_pNext);
			}
		}

		// 完了の通知は後続を流したあと(JobWorker::Execute と同じ理由)
		_context.NotifyJobFinished();
		_context.FinishPendingJob();
	}
}
