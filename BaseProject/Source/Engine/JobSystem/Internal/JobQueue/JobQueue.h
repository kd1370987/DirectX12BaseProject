#pragma once
namespace Engine::Thread
{
	class JobQueue
	{
	public:

		void Push(std::function<void()> a_job);

		bool TryPop(std::function<void()>& a_outJob);
		bool TrySteal(std::function<void()>& a_outJob);

		// 待機側の述語から呼ばれる。
		// 他スレッドが Push/Pop している最中の deque を素で読むと競合するため、
		// 参照するだけでもロックを取る
		bool HasJob() const
		{
			std::lock_guard _lock(m_mutex);
			return !m_jobs.empty();
		}

	private:
		std::deque<std::function<void()>> m_jobs = {};		// タスク
		mutable std::mutex m_mutex;
	};
}