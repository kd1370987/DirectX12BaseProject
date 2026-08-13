#pragma once
namespace Engine::Thread
{
	class JobQueue
	{
	public:

		void Push(std::function<void()> a_job);

		bool TryPop(std::function<void()>& a_outJob);
		bool TrySteal(std::function<void()>& a_outJob);

		bool HasJob() const { return !m_jobs.empty(); }

	private:
		std::deque<std::function<void()>> m_jobs = {};		// タスク
		std::mutex m_mutex;
	};
}