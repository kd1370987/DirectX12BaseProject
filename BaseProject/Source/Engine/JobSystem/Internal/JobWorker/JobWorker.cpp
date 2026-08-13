#include "JobWorker.h"

#include "../JobContext.h"

namespace Engine::Thread
{
	void JobWorker::Start(JobContext* a_pJobContext, uint32_t a_workerID)
	{
		m_pContext = a_pJobContext;
		m_workerID = a_workerID;
		m_isRunning.store(true, std::memory_order_release);

		// 新規にOSスレッドを立ち上げて、Run()を実行
		m_thread = std::thread([this] {this->Run(); });
	}
	void JobWorker::RequestStop()
	{
		if (!m_pContext) return;

		// 停止フラグは WaitForJob() の述語が見る値なので、
		// 必ず待機側と同じミューテックスの下で書き換える。
		// ロックを取らずに書き換えて通知すると、
		// 「述語が false と判定された直後・眠りにつく前」に通知がすれ違い、
		// 二度と起きずに Join() が返らなくなる(lost wakeup)
		{
			std::lock_guard _lock(m_pContext->jobAvailableMutex);
			m_isRunning.store(false, std::memory_order_release);
		}

		// 条件変数は全ワーカーで共有しているため、
		// 自分だけを狙って起こすことができない。まとめて起こして各自に判定させる
		m_pContext->NotifyAllWorkers();
	}
	void JobWorker::Join()
	{
		if (m_thread.joinable())
		{
			m_thread.join(); // このスレッドが来るまで、現在のスレッドを待つ
		}
		m_pContext = nullptr;
	}
	void JobWorker::PushJob(std::function<void()>&& a_job)
	{
		m_jobQueue.Push(std::move(a_job));

		// 積み終わってから起こす。
		// 先に起こすと、起きたワーカーが空のキューを見て寝直すだけになる
		m_pContext->NotifyJobAvailable();
	}
	void Engine::Thread::JobWorker::Run()
	{
		while (m_isRunning.load(std::memory_order_acquire))
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

		// 停止要求と入れ違いで積まれた分の回収。
		// ここで拾わないと未完了カウンタが減らず、
		// 以降の WaitForAll() が永久に返らなくなる
		std::function<void()> _restJob;
		while (m_jobQueue.TryPop(_restJob))
		{
			Execute(_restJob);
		}
	}
	void JobWorker::Execute(std::function<void()>& a_job)
	{
		// 取り出した時点で「待っている仕事」ではなくなる
		m_pContext->OnJobDequeued();

		// ジョブから例外が抜けるとスレッド関数の外まで飛んで std::terminate になる。
		// アセットのロードはファイル欠損やパース失敗で普通に投げるので、
		// ここで受け止めてジョブ1件の失敗に閉じ込める
		try
		{
			a_job();
		}
		catch (const std::exception& _e)
		{
			ENGINE_WARNING("[JobSystem] ジョブが例外で終了しました : %s", _e.what());
		}
		catch (...)
		{
			ENGINE_WARNING("[JobSystem] ジョブが不明な例外で終了しました");
		}

		// 例外で終わっても必ず減らす。
		// ここを飛ばすと WaitForAll() が永久に返らなくなる
		m_pContext->FinishPendingJob();
	}
	bool JobWorker::TrySteal(std::function<void()>& a_outJob)
	{
		// 全ワーカーから盗めるタスクを探す。
		// 停止済みのワーカーも対象にする : 除外すると、
		// そのキューに残った仕事を誰も引き取れないまま捨てることになる
		for (auto* _pWorker : m_pContext->pJobWorker)
		{
			if (_pWorker == this) continue;
			if (_pWorker->RefQueue().TrySteal(a_outJob)) return true;
		}

		return false;
	}
	void JobWorker::WaitForJob()
	{
		std::unique_lock _lock(m_pContext->jobAvailableMutex);

		m_pContext->jobAvailableCondition.wait(
			_lock,
			[this]()
			{
				// 自分のキューではなく全体の残数を見る。
				// 他のワーカーに仕事が溜まっていれば起きて盗みに行く
				return !m_isRunning.load(std::memory_order_acquire)
					|| m_pContext->queuedJobCount.load(std::memory_order_acquire) > 0;
			}
		);
	}
}
