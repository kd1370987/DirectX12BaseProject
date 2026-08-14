#include "JobQueue.h"
namespace Engine::Thread
{
	void Engine::Thread::JobQueue::Push(Job* a_pJob)
	{
		std::lock_guard _lock(m_mutex);
		m_jobs.push_back(a_pJob);
	}
	bool JobQueue::TryPop(Job*& a_pOutJob)
	{
		std::lock_guard _lock(m_mutex);
		if (m_jobs.empty()) return false;

		// 処理するタスクは後ろから取得
		a_pOutJob = m_jobs.back();
		m_jobs.pop_back();

		return true;
	}
	bool JobQueue::TrySteal(Job*& a_pOutJob)
	{
		std::lock_guard _lock(m_mutex);
		if (m_jobs.empty()) return false;

		// 盗まれるタスクは前から渡す
		a_pOutJob = m_jobs.front();
		m_jobs.pop_front();

		return true;
	}
}