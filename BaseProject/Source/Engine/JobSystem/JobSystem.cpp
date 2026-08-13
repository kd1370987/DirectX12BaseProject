#include "JobSystem.h"

#include "Internal/JobContext.h"
#include "Internal/JobWorker/JobWorker.h"

namespace Engine::Thread
{
	JobSystem::JobSystem()
	{}
	JobSystem::~JobSystem()
	{}
	void Engine::Thread::JobSystem::Init(uint32_t a_threadCount)
	{
		ENGINE_LOG("[Init] JobSystemがスレッド数 : %d で初期化されました",(int)a_threadCount);

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

		// すべてのスレッドを起動
		for (auto& _worker : m_jobWorkers)
		{
			_worker->Start(m_upJobContext.get());
		}
	}

	void Engine::Thread::JobSystem::Release()
	{
		WaitForAll();

		for (auto& _worker : m_jobWorkers)
		{
			_worker->Stop();
		}
	}

	void Engine::Thread::JobSystem::PushJob(std::function<void()>&& a_job)
	{
		// 積む先がないとカウンタだけ増えて WaitForAll() が返らなくなるため、
		// 未初期化ならカウンタを触らずに弾く
		if (m_jobWorkers.empty())
		{
			ENGINE_WARNING("[JobSystem] 初期化前にPushJobが呼ばれました");
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