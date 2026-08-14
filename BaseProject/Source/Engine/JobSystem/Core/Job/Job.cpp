#include "Job.h"

namespace Engine::Thread
{
	void Job::Reset()
	{
		std::lock_guard _lock(m_continuationMutex);

		task = nullptr;
		waitingCount.store(0, std::memory_order_relaxed);

		// clear() は確保済みの容量を手放さないので、
		// プールから取り直すたびに確保し直すことにはならない
		m_continuations.clear();

		// ここから「実行中」になる
		m_isFinished.store(false, std::memory_order_seq_cst);
	}

	bool Job::AddContinuation(Job* a_pJob)
	{
		if (a_pJob == nullptr) return false;

		std::lock_guard _lock(m_continuationMutex);

		// すでに終わっている : 登録しても通知は飛ばない。
		// 呼び出し側にその場で待ち数を減らしてもらう
		if (m_isFinished.load(std::memory_order_acquire)) return false;

		m_continuations.push_back(a_pJob);
		return true;
	}

	void Job::FinishAndTakeContinuations(std::vector<Job*>& a_outContinuations)
	{
		std::lock_guard _lock(m_continuationMutex);

		// フラグを立てるのと後続の取り出しは不可分に行う。
		// 分けると、その隙間に入った AddContinuation の登録を取りこぼす
		m_isFinished.store(true, std::memory_order_seq_cst);

		// swap ではなく代入で渡す。
		// swap だと呼び出し側の使い回し用の容量がこちらへ移ってしまい、
		// 次にこのジョブを使うときに確保し直すことになる
		a_outContinuations.assign(m_continuations.begin(), m_continuations.end());
		m_continuations.clear();
	}
}
