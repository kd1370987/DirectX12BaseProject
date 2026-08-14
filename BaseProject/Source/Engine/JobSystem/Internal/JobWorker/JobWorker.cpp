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
	Job* JobWorker::CreateJob(std::function<void()>&& a_task)
	{
		Job* _pJob = m_jobPool.AllocateJob();
		_pJob->task = std::move(a_task);

		// まだキューへは積まない。
		// 依存を張り終えてから PushReadyJob() で流す
		return _pJob;
	}
	void JobWorker::PushReadyJob(Job* a_pJob)
	{
		if (a_pJob == nullptr) return;

		// キューへ積む前に数える。
		// 積んでから増やすと、走り出したワーカーが増える前のカウンタを減らしてしまう
		m_pContext->AddQueuedJob();

		m_jobQueue.Push(a_pJob);

		// 積み終わってから起こす。
		// 先に起こすと、起きたワーカーが空のキューを見て寝直すだけになる
		m_pContext->NotifyJobAvailable();
	}
	void Engine::Thread::JobWorker::Run()
	{
		while (m_isRunning.load(std::memory_order_acquire))
		{
			// ジョブの入れ物準備
			Job* _pJob = nullptr;

			// 自身のキューからタスクを取得
			if (m_jobQueue.TryPop(_pJob))
			{
				Execute(_pJob);
				continue;
			}

			// ほかのWorkerから盗む
			if (TrySteal(_pJob))
			{
				Execute(_pJob);
				continue;
			}

			// ジョブが来るまで待機
			WaitForJob();
		}

		// 停止要求と入れ違いで積まれた分の回収。
		// ここで拾わないと未完了カウンタが減らず、
		// 以降の WaitForAll() が永久に返らなくなる
		Job* _pRestJob = nullptr;
		while (m_jobQueue.TryPop(_pRestJob))
		{
			Execute(_pRestJob);
		}
	}
	void JobWorker::Execute(Job* a_pJob)
	{
		if (a_pJob == nullptr) return;

		// 取り出した時点で「待っている仕事」ではなくなる
		m_pContext->OnJobDequeued();

		// ジョブから例外が抜けるとスレッド関数の外まで飛んで std::terminate になる。
		// アセットのロードはファイル欠損やパース失敗で普通に投げるので、
		// ここで受け止めてジョブ1件の失敗に閉じ込める
		try
		{
			if (a_pJob->task) a_pJob->task();
		}
		catch (const std::exception& _e)
		{
			ENGINE_WARNING("[JobSystem] ジョブが例外で終了しました : %s", _e.what());
		}
		catch (...)
		{
			ENGINE_WARNING("[JobSystem] ジョブが不明な例外で終了しました");
		}

		// 例外で終わっても後続は必ず動かす。
		// ここを飛ばすと、このジョブを待っているものが永久に起きてこない
		FinishJob(a_pJob);

		// 完了通知は後続を流したあとに行う。
		// 先に減らすと、後続がキューへ入る前に未完了数が0になり、
		// WaitForAll() がまだ仕事が残っているのに返ってしまう
		m_pContext->FinishPendingJob();
	}
	void JobWorker::FinishJob(Job* a_pJob)
	{
		// 完了印を付けつつ後続を引き取る。
		// この2つを不可分に行わないと、隙間に入った依存登録を取りこぼす
		std::array<Job*, Job::MAX_CONTINUATION_COUNT> _continuations = {};
		const uint32_t _count = a_pJob->FinishAndTakeContinuations(_continuations);

		for (uint32_t _i = 0; _i < _count; ++_i)
		{
			Job* _pNext = _continuations[_i];
			if (_pNext == nullptr) continue;

			// 待ち数を0にしたスレッドだけがキューへ積む。
			// fetch_sub の戻り値で判定しないと、
			// 同時に最後の1を減らした複数スレッドが二重に積んでしまう
			if (_pNext->waitingCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				// 自分のキューへ積む : 直前に触ったデータが暖まっている
				PushReadyJob(_pNext);
			}
		}
	}
	bool JobWorker::TrySteal(Job*& a_pOutJob)
	{
		// 全ワーカーから盗めるタスクを探す。
		// 停止済みのワーカーも対象にする : 除外すると、
		// そのキューに残った仕事を誰も引き取れないまま捨てることになる
		for (auto* _pWorker : m_pContext->pJobWorker)
		{
			if (_pWorker == this) continue;
			if (_pWorker->RefQueue().TrySteal(a_pOutJob)) return true;
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
