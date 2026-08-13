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
		auto& _worker = m_jobWorkers[m_nextWorker];
		_worker->PushJob(std::move(a_job));

		// 次の割り振りを決める
		++m_nextWorker;
		if (m_nextWorker >= m_jobWorkers.size()) m_nextWorker = 0;
	}

	void Engine::Thread::JobSystem::WaitForAll()
	{
		std::unique_lock _lock(m_jobFinishedMutex);

		m_jobFinishedCondition.wait(
			_lock,
			[this]()
			{
				for (auto& _worker : m_jobWorkers)
				{
					if (!_worker->IsIdle()) return false;
				}
				return true;
			}
		);
	}
}