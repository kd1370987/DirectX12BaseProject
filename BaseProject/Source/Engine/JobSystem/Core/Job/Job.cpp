#include "Job.h"

namespace Engine::Thread
{
	void Job::Reset()
	{
		std::lock_guard _lock(m_continuationMutex);

		task = nullptr;
		waitingCount.store(0, std::memory_order_relaxed);

		m_continuations = {};
		m_continuationCount = 0;

		// ここから「実行中」になる
		m_isFinished.store(false, std::memory_order_release);
	}

	bool Job::AddContinuation(Job* a_pJob)
	{
		if (a_pJob == nullptr) return false;

		std::lock_guard _lock(m_continuationMutex);

		// すでに終わっている : 登録しても通知は飛ばない。
		// 呼び出し側にその場で待ち数を減らしてもらう
		if (m_isFinished.load(std::memory_order_acquire)) return false;

		if (m_continuationCount >= MAX_CONTINUATION_COUNT)
		{
			// 溢れた分は待たせたままにするとデッドロックになるため、
			// 「解決済み」として扱って先に進める。
			// 依存の張り方を見直すこと
			ENGINE_WARNING("[JobSystem] 後続ジョブの上限(%u)を超えたため依存を張れませんでした", MAX_CONTINUATION_COUNT);
			return false;
		}

		m_continuations[m_continuationCount] = a_pJob;
		++m_continuationCount;
		return true;
	}

	uint32_t Job::FinishAndTakeContinuations(std::array<Job*, MAX_CONTINUATION_COUNT>& a_outContinuations)
	{
		std::lock_guard _lock(m_continuationMutex);

		// フラグを立てるのと後続の取り出しは不可分に行う。
		// 分けると、その隙間に入った AddContinuation の登録を取りこぼす
		m_isFinished.store(true, std::memory_order_release);

		a_outContinuations = m_continuations;

		const uint32_t _count = m_continuationCount;
		m_continuationCount = 0;

		return _count;
	}
}
