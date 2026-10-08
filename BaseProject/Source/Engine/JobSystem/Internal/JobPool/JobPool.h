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
	//
	// ・まだ終わっていないスロットは飛ばす
	//     一周した先が実行中のジョブだと、Reset が実行中のラムダ(キャプチャ)を
	//     次のジョブの中身で上書きする。アセットのロードのように長いジョブの間に
	//     ECS のフレームジョブが一周ぶん積まれると普通に起きる。
	//     上書きされたロードジョブは、キャプチャした this が別の物(SystemTask など)を指したまま
	//     続きを実行し、そこへ書き込んでメモリを壊していた(Desert_02 のクエリキャッシュでの停止)。
	//     そのため、終わっているスロットだけを取る。
	//
	// ・スロットの取得は完了印の付け替え(true → false)を CAS で行う
	//     同じスロットを2つのスレッドが同時に取らないようにするため。
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
			bool _isWarned = false;

			while (true)
			{
				// 一周ぶん探す。未完了のスロット(実行中・依存待ち・キュー待ち)は飛ばす
				for (uint32_t _attempt = 0; _attempt < MAX_JOB_COUNT; ++_attempt)
				{
					// 同じワーカーへ複数スレッドから積まれるため、
					// 添え字の取得は不可分に行う
					const uint32_t _count = m_allocatedJobCount.fetch_add(1, std::memory_order_relaxed);
					Job* _pJob = &m_jobs[_count & (MAX_JOB_COUNT - 1u)];

					// 終わっているスロットだけを取る : 取った時点で「未完了」にする
					bool _isFinished = true;
					if (!_pJob->m_isFinished.compare_exchange_strong(_isFinished, false, std::memory_order_acq_rel)) continue;

					_pJob->Reset();
					return _pJob;
				}

				//------------------------------------------------------------------
				// 全スロットが未完了 : 上書きはせず、どれかが終わるまで譲って待つ。
				// 待っている間もほかのワーカーがジョブを消化するので、いずれ空く
				//------------------------------------------------------------------
				if (!_isWarned)
				{
					ENGINE_WARNING("[JobSystem] ジョブプールの全スロット(%u)が未完了のため、空くまで待ちます", MAX_JOB_COUNT);
					_isWarned = true;
				}
				std::this_thread::yield();
			}
		}

	private:

		// ジョブの実体
		std::unique_ptr<Job[]>	m_jobs;
		std::atomic<uint32_t>	m_allocatedJobCount = 0u;
	};
}
