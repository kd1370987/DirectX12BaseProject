#pragma once
namespace Engine::Thread
{
	class JobWorker;

	//==========================================================================================
	// ジョブシステムとワーカースレッドの共有データ
	//
	// 完了待ちは「各キューが空か」ではなく未完了ジョブ数で数える。
	// キューの中身だけを見ると、ワーカーがジョブを取り出した後・実行を始める前の隙間で
	// 「どこにも仕事がない」状態に見えてしまい、待機側が素通りしてしまうため。
	//
	// カウンタは投入側(PushJob)で増やし、実行し終えたワーカーが減らす。
	// ジョブの中からさらにジョブを積む場合も、親が減らす前に子が増えるので
	// 途中で 0 に落ちることはない。
	//==========================================================================================
	struct JobContext
	{
		std::vector<JobWorker*> pJobWorker = {};

		// ---- 完了待ち ----
		std::atomic<uint32_t>	pendingJobCount = 0;	// 未完了ジョブ数(キュー待ち + 実行中)
		std::condition_variable	finishedCondition;		// 全ジョブ完了の通知
		std::mutex				finishedMutex;

		/// <summary>
		/// ジョブの投入を通知する
		/// 実際にキューへ積む前に呼ぶこと。
		/// 後に呼ぶと、積んだ直後に走り出したワーカーが増える前のカウンタを減らしてしまう
		/// </summary>
		void AddPendingJob()
		{
			pendingJobCount.fetch_add(1, std::memory_order_relaxed);
		}

		/// <summary>
		/// ジョブの完了を通知する
		/// </summary>
		void FinishPendingJob()
		{
			// 最後の1件を減らしたスレッドだけが待機側を起こす
			if (pendingJobCount.fetch_sub(1, std::memory_order_acq_rel) != 1) return;

			// 待機側は finishedMutex を握った状態で述語を評価するため、
			// ここでロックを取らずに通知すると
			// 「述語が false と判定された直後・眠りにつく前」に通知がすれ違い、
			// 二度と起きなくなる(lost wakeup)。必ずロックを取ってから通知する
			{
				std::lock_guard _lock(finishedMutex);
			}
			finishedCondition.notify_all();
		}

		/// <summary>
		/// 全ジョブが終わるまで待機する
		/// </summary>
		void WaitForAllJobs()
		{
			std::unique_lock _lock(finishedMutex);

			finishedCondition.wait(
				_lock,
				[this]()
				{
					return pendingJobCount.load(std::memory_order_acquire) == 0;
				}
			);
		}
	};
}
