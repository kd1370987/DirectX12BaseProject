#include "JobWorker.h"

#include "../JobContext.h"

namespace Engine::Thread
{
	void JobWorker::Start(JobContext* a_pJobContext)
	{
		m_pContext = a_pJobContext;
		m_isRunning = true;

		// 新規にOSスレッドを立ち上げて、Run()を実行
		m_thread = std::thread([this] {this->Run(); });
	}
	void JobWorker::Stop()
	{
		m_isRunning = false;
		m_condition.notify_one();
		if (m_thread.joinable())
		{
			m_thread.join(); // このスレッドが来るまで、現在のスレッドを待つ
		}
		m_pContext = nullptr;
	}
	void JobWorker::PushJob(std::function<void()>&& a_job)
	{
		m_jobQueue.Push(a_job);
		m_condition.notify_one();
	}
	bool JobWorker::IsIdle() const
	{
		return !m_isWorking.load(std::memory_order_acquire) && !m_jobQueue.HasJob();
	}
	void Engine::Thread::JobWorker::Run()
	{
		while (m_isRunning)
		{
			// ジョブの入れ物準備
			std::function<void()> _job;

			// 自身のキューからタスクを取得
			if (m_jobQueue.TryPop(_job))
			{
				Execute(_job);
				continue;
			}
			
			// ほかのWorkerから盗む
			if (TrySteal(_job))
			{
				Execute(_job);
				continue;
			}
			
			// ジョブが来るまで待機
			WaitForJob();
		}
	}
	void JobWorker::Execute(std::function<void()>& a_job)
	{
		m_isWorking = true;
		a_job();
		m_isWorking = false;
	}
	bool JobWorker::TrySteal(std::function<void()>& a_outJob)
	{
		// 全ワーカーから盗めるタスクを探す
		for (auto* _pWorker : m_pContext->pJobWorker)
		{
			if (_pWorker == this) continue;
			if (!_pWorker->IsRunning()) continue;
			if (_pWorker->RefQueue().TrySteal(a_outJob)) return true;
		}

		return false;
	}
	void JobWorker::WaitForJob()
	{
		std::unique_lock _lock(m_mutex);

		m_condition.wait(
			_lock,
			[this]()
			{
				return !m_isRunning || m_jobQueue.HasJob();
			}
		);
	}
}