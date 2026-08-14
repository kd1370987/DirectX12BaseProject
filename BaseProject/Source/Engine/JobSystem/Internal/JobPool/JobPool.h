#pragma once
namespace Engine::Thread
{

	static constexpr uint32_t MAX_JOB_COUNT = 4096;

	class JobPool
	{
	public:

		JobPool() { m_jobs.resize(MAX_JOB_COUNT); }

		Job* AllocateJob(void)
		{
			if (m_allocatedJobCount - 1u >= MAX_JOB_COUNT) return nullptr;
			const uint32_t _index = m_allocatedJobCount++;
			return &m_jobs[_index];
		}

	private:

		// ジョブの実体
		std::vector<Job> m_jobs = {};
		uint32_t m_allocatedJobCount = 0u;
	};
}