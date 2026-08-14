#pragma once
namespace Engine::Thread
{
	class JobWorker;

	//==========================================================================================
	// ジョブシステムとワーカースレッドの共有データ
	//
	// ジョブの件数を2種類のカウンタで持つ。
	//
	//   pendingJobCount : 投入されてから実行し終わるまで (キュー待ち + 実行中)
	//                     -> WaitForAll() の判定に使う
	//   queuedJobCount  : 投入されてからワーカーに取り出されるまで (キュー待ちのみ)
	//                     -> 寝ているワーカーを起こすかどうかの判定に使う
	//
	// 完了待ちを「各キューが空か」で見ないのは、
	// ワーカーがジョブを取り出した後・実行を始める前の隙間で
	// 「どこにも仕事がない」状態に見えてしまい、待機側が素通りするため。
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

		// ---- 個別ジョブの完了待ち ----
		// WaitForAll() は「システム全体のジョブが尽きるまで」しか待てない。
		// モデルロードのような長いジョブが同時に走っているので、
		// フレーム内の同期(ECS のフェーズ終わり待ちなど)には使えない。
		// そのため「このジョブが終わるまで」だけを待つ経路を別に用意する
		std::atomic<uint32_t>	jobWaiterCount = 0;		// WaitFor() で待っているスレッド数
		std::condition_variable	jobFinishedCondition;	// ジョブ1件の完了通知
		std::mutex				jobFinishedMutex;

		// ---- ジョブ待ち ----
		// ワーカーは全員この1つの条件変数で待つ。
		// ワーカーごとに条件変数を持って自分のキューだけを見ていると、
		// 他のワーカーに仕事が溜まっていても寝たままになり、
		// ワークスティールが「たまたま起きていたとき」しか働かない
		std::atomic<uint32_t>	queuedJobCount = 0;		// まだ誰にも取り出されていないジョブ数
		std::condition_variable	jobAvailableCondition;	// 仕事が来たことの通知
		std::mutex				jobAvailableMutex;

		/// <summary>
		/// ジョブの受付を通知する : 完了待ちの総数を増やす
		///
		/// 依存待ちのジョブはすぐにはキューへ入らないため、
		/// 「受け付けた数」と「キューに積まれた数」は別々に数える。
		/// まとめて数えると、依存が解けるまでの間ずっと
		/// 「仕事がある」と見えてワーカーが空回りし続ける
		/// </summary>
		void AddPendingJob()
		{
			pendingJobCount.fetch_add(1, std::memory_order_relaxed);
		}

		/// <summary>
		/// キューへの投入を通知する
		/// 実際にキューへ積む前に呼ぶこと。
		/// 後に呼ぶと、積んだ直後に走り出したワーカーが増える前のカウンタを減らしてしまう
		/// </summary>
		void AddQueuedJob()
		{
			queuedJobCount.fetch_add(1, std::memory_order_release);
		}

		/// <summary>
		/// ワーカーがキューから取り出したときに呼ぶ
		/// 「待っている仕事」ではなくなるので、起こす判断からは外れる
		/// </summary>
		void OnJobDequeued()
		{
			queuedJobCount.fetch_sub(1, std::memory_order_acq_rel);
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
		/// ジョブ1件の完了を知らせる : ワーカーが1件終えるたびに呼ぶ
		///
		/// ロードジョブのように大量に流れるものでミューテックスを踏まないよう、
		/// 待っている人がいないときは何もしない。
		///
		/// この「先に完了印を付けてから待ち人数を見る」と
		/// WaitForJob() 側の「先に待ち人数を増やしてから完了印を見る」は
		/// 互いにすれ違う形になっているため、
		/// 両方の読み書きを seq_cst にして1本の順序へ載せてある。
		/// これで「通知が飛ばず、かつ完了印も見えない」という組み合わせは起きない
		/// </summary>
		void NotifyJobFinished()
		{
			if (jobWaiterCount.load(std::memory_order_seq_cst) == 0) return;

			// 待機側は jobFinishedMutex を握って述語を評価するため、
			// ロックを一度通してから通知しないと lost wakeup になる
			{
				std::lock_guard _lock(jobFinishedMutex);
			}
			jobFinishedCondition.notify_all();
		}

		/// <summary>
		/// 指定したジョブが終わるまで待機する
		/// </summary>
		template<typename FinishedPredicateFnc>
		void WaitForJobFinished(FinishedPredicateFnc&& a_isFinished)
		{
			// 待ち人数は「完了印を見る前」に増やしきる。
			// 後にすると、その隙間に終わったジョブの通知を取りこぼす
			jobWaiterCount.fetch_add(1, std::memory_order_seq_cst);

			{
				std::unique_lock _lock(jobFinishedMutex);
				jobFinishedCondition.wait(_lock, a_isFinished);
			}

			jobWaiterCount.fetch_sub(1, std::memory_order_seq_cst);
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

		/// <summary>
		/// 仕事が入ったことを知らせて、寝ているワーカーを1つ起こす
		/// キューへ積み終わってから呼ぶこと
		/// </summary>
		void NotifyJobAvailable()
		{
			// 起こし損ねを防ぐため、待機側と同じミューテックスを一度通してから通知する
			{
				std::lock_guard _lock(jobAvailableMutex);
			}
			jobAvailableCondition.notify_one();
		}

		/// <summary>
		/// 全ワーカーを起こす : 停止を伝えるときに使う
		/// どのスレッドが寝ているか特定できないため、まとめて起こす
		/// </summary>
		void NotifyAllWorkers()
		{
			{
				std::lock_guard _lock(jobAvailableMutex);
			}
			jobAvailableCondition.notify_all();
		}
	};
}
