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
		// 停止フラグは WaitForJob() の述語が見る値なので、
		// 必ず待機側と同じミューテックスの下で書き換える。
		// ロックを取らずに書き換えて通知すると、
		// 「述語が false と判定された直後・眠りにつく前」に通知がすれ違い、
		// 二度と起きずに join() が返らなくなる(lost wakeup)
		{
			std::lock_guard _lock(m_mutex);
			m_isRunning = false;
		}
		m_condition.notify_one();

		if (m_thread.joinable())
		{
			m_thread.join(); // このスレッドが来るまで、現在のスレッドを待つ
		}
		m_pContext = nullptr;
	}
	void JobWorker::PushJob(std::function<void()>&& a_job)
	{
		m_jobQueue.Push(std::move(a_job));

		// キューの中身も述語が見る値。
		// キュー自身のロックとは別物なので、通知の前に待機側のミューテックスを
		// 一度通しておかないと Stop() と同じすれ違いでジョブが放置される
		{
			std::lock_guard _lock(m_mutex);
		}
		m_condition.notify_one();
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
		a_job();

		// 実行し終えたので未完了数を減らす。
		// これを通さないと WaitForAll() 側が永久に待ち続けるため、
		// どの経路で取得したジョブでも必ずここを通すこと
		m_pContext->FinishPendingJob();
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