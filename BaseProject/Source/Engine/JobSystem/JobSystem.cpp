#include "JobSystem.h"

#include "Internal/JobContext.h"
#include "Internal/JobWorker/JobWorker.h"

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

		// コンテキスト作成
		m_upJobContext = std::make_unique<JobContext>();

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
			_worker->RequestStop();
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
	}

	void Engine::Thread::JobSystem::PushJob(std::function<void()>&& a_job)
	{
		// 停止中や未初期化で積むと、カウンタだけ増えて誰も処理しない。
		// WaitForAll() が返らなくなるので、カウンタを触る前に弾く
		if (!m_isRunning.load(std::memory_order_acquire) || m_jobWorkers.empty())
		{
			ENGINE_WARNING("[JobSystem] 停止中または未初期化のためジョブを破棄しました");
			return;
		}

		// キューへ積む前にカウンタを増やす。
		// 積んでから増やすと、走り出したワーカーが増える前のカウンタを減らしてしまう
		m_upJobContext->AddPendingJob();

		// 割り当て先の決定。
		// ジョブの中からジョブを積むとここが複数スレッドから同時に走るため、
		// 「加算」と「読み出し」を分けると割り込まれて範囲外の添え字を引く。
		// fetch_add で取った値をそのまま自分の取り分として使う
		const uint32_t _workerIndex =
			m_nextWorker.fetch_add(1, std::memory_order_relaxed)
			% static_cast<uint32_t>(m_jobWorkers.size());

		m_jobWorkers[_workerIndex]->PushJob(std::move(a_job));
	}

	void Engine::Thread::JobSystem::WaitForAll()
	{
		if (!m_upJobContext) return;

		m_upJobContext->WaitForAllJobs();
	}
}
