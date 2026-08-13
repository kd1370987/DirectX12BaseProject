#include "JobQueue.h"
namespace Engine::Thread
{
	void Engine::Thread::JobQueue::Push(std::function<void()> a_job)
	{
		std::lock_guard _lock(m_mutex);
		m_jobs.push_back(a_job);
	}
	bool JobQueue::TryPop(std::function<void()>&a_outJob)
	{
		std::lock_guard _lock(m_mutex);
		if (m_jobs.empty()) return false;

		// 処理するタスクは後ろから取得
		a_outJob = std::move(m_jobs.back());
		m_jobs.pop_back();

		return true;
	}
	bool JobQueue::TrySteal(std::function<void()>& a_outJob)
	{
		std::lock_guard _lock(m_mutex);
		if (m_jobs.empty()) return false;

		// 盗まれるタスクは前から渡す
		a_outJob = std::move(m_jobs.front());
		m_jobs.pop_front();

		return false;
	}
}