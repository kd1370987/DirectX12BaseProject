#pragma once
namespace Engine::Thread
{
	// 1つのプールが抱えるジョブ数 : リングで使い回すため2の冪であること
	static constexpr uint32_t MAX_JOB_COUNT = 4096;

	//==========================================================================================
	// ジョブの実体置き場
	//
	// リングバッファとして使い回す。確保のたびに new しないかわりに、
	// 「同時に生きていられるジョブは1ワーカーあたり MAX_JOB_COUNT 件まで」という制約が付く。
	// これを超えるとまだ終わっていないジョブのスロットを踏み潰すため、
	// 踏んだ場合は警告を出す。
	//
	// Job はミューテックスとアトミックを持っていてコピーもムーブもできないので、
	// vector ではなく配列で確保する
	//==========================================================================================
	class JobPool
	{
	public:

		JobPool() : m_jobs(std::make_unique<Job[]>(MAX_JOB_COUNT)) {}

		/// <summary>
		/// ジョブを1つ取り出す
		/// 中身は初期化されて返る
		/// </summary>
		Job* AllocateJob()
		{
			// 同じワーカーへ複数スレッドから積まれるため、
			// 添え字の取得は不可分に行う
			const uint32_t _count = m_allocatedJobCount.fetch_add(1, std::memory_order_relaxed);
			Job* _pJob = &m_jobs[_count & (MAX_JOB_COUNT - 1u)];

			// まだ終わっていないジョブを踏んでいないか
			if (!_pJob->m_isFinished.load(std::memory_order_acquire))
			{
				ENGINE_WARNING("[JobSystem] ジョブプールが一周し、未完了のジョブを上書きしました(上限 %u)", MAX_JOB_COUNT);
			}

			_pJob->Reset();
			return _pJob;
		}

	private:

		// ジョブの実体
		std::unique_ptr<Job[]>	m_jobs;
		std::atomic<uint32_t>	m_allocatedJobCount = 0u;
	};
}
