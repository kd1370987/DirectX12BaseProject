#include "ThreadProfiler.h"

namespace Engine::Thread
{
	namespace
	{
		constexpr size_t STATE_COUNT = static_cast<size_t>(EThreadState::Count);

		// 時刻 : steady_clock の刻み(ナノ秒)のまま整数で持つ
		int64_t Now()
		{
			return std::chrono::steady_clock::now().time_since_epoch().count();
		}

		// 刻みをミリ秒へ
		double TicksToMs(double a_ticks)
		{
			using Period = std::chrono::steady_clock::period;
			return a_ticks * 1000.0 * static_cast<double>(Period::num) / static_cast<double>(Period::den);
		}
	}

	//==========================================================================================
	// スレッド1本ぶんの記録
	//
	// 書くのは持ち主のスレッドだけ、読むのは EndFrame のメインスレッドだけ。
	//
	// 状態の切り替えでは「閉じた区間の合計」「今の状態」「今の状態に入った時刻」の3つを
	// まとめて書き換えるが、読む側がその途中を見ると、同じ区間を二重に数えたり
	// 取りこぼしたりする。なので書き込みの前後で通し番号を奇数 -> 偶数と進め、
	// 読む側は「偶数で、読む前後で変わっていない」ときだけ採用する(シーケンスロック)。
	// 書く側は待たないので、ジョブの実行を止めることはない
	//==========================================================================================
	struct alignas(64) ThreadActivity
	{
		std::atomic<uint32_t> sequence = 0;						// 書き換え中は奇数
		std::atomic<int64_t>  stateTicks[STATE_COUNT] = {};		// 状態ごとの、閉じた区間の合計
		std::atomic<int64_t>  stateBegin = 0;					// 今の状態に入った時刻
		std::atomic<uint8_t>  state = static_cast<uint8_t>(EThreadState::Idle);

		// 件数は1つずつ独立に読めればよいので、シーケンスロックの外に置く
		std::atomic<uint64_t> jobCount = 0;
		std::atomic<uint64_t> stealCount = 0;

		/// <summary>
		/// 状態を切り替える (持ち主のスレッドのみ)
		/// </summary>
		EThreadState Transition(EThreadState a_next, int64_t a_now)
		{
			const auto _prev = static_cast<EThreadState>(state.load(std::memory_order_relaxed));
			if (_prev == a_next) return _prev;

			const uint32_t _seq = sequence.load(std::memory_order_relaxed);
			sequence.store(_seq + 1, std::memory_order_relaxed);
			// 奇数にしたことが、以下の書き込みより先に見えるようにする
			std::atomic_thread_fence(std::memory_order_release);

			auto& _total = stateTicks[static_cast<size_t>(_prev)];
			_total.store(
				_total.load(std::memory_order_relaxed) + (a_now - stateBegin.load(std::memory_order_relaxed)),
				std::memory_order_relaxed);
			stateBegin.store(a_now, std::memory_order_relaxed);
			state.store(static_cast<uint8_t>(a_next), std::memory_order_relaxed);

			sequence.store(_seq + 2, std::memory_order_release);
			return _prev;
		}

		/// <summary>
		/// 状態ごとの合計を、今の状態の途中経過まで含めて読む (メインスレッドのみ)
		/// </summary>
		void Sample(int64_t a_now, int64_t (&a_outTicks)[STATE_COUNT]) const
		{
			int64_t _begin = 0;
			uint8_t _state = 0;

			while (true)
			{
				const uint32_t _seq = sequence.load(std::memory_order_acquire);

				// 書き換えの途中 : すぐ終わるので待つ
				if (_seq & 1u)
				{
					_mm_pause();
					continue;
				}

				for (size_t _i = 0; _i < STATE_COUNT; ++_i)
				{
					a_outTicks[_i] = stateTicks[_i].load(std::memory_order_relaxed);
				}
				_begin = stateBegin.load(std::memory_order_relaxed);
				_state = state.load(std::memory_order_relaxed);

				// 上の読み込みが、下の通し番号の読み直しより先に済むようにする
				std::atomic_thread_fence(std::memory_order_acquire);
				if (sequence.load(std::memory_order_relaxed) == _seq) break;
			}

			// 実行中の区間は、今の時刻で区切って足す。
			// 読み取りの直前に切り替わった場合は a_now より後の時刻が入っていることがあるので、負にしない
			a_outTicks[_state] += (std::max)(int64_t(0), a_now - _begin);
		}
	};

	namespace
	{
		// 呼んだスレッドの記録 : 登録していないスレッドは nullptr のまま
		thread_local ThreadActivity* t_pActivity = nullptr;
	}

	ThreadProfiler::ThreadProfiler()
	{}
	ThreadProfiler::~ThreadProfiler()
	{}

	void ThreadProfiler::Init(uint32_t a_workerCount)
	{
		m_threadCount = a_workerCount + 1;
		m_upActivities = std::make_unique<ThreadActivity[]>(m_threadCount);
		m_accumulators.assign(m_threadCount, ThreadAccumulator{});

		// 状態に入った時刻を今にしておく。
		// 0 のままだと、まだ起動していないワーカーを読んだときに
		// 「時計の起点からずっと Idle だった」ことになる
		const int64_t _now = Now();
		for (uint32_t _i = 0; _i < m_threadCount; ++_i)
		{
			m_upActivities[_i].stateBegin.store(_now, std::memory_order_relaxed);
		}

		m_hasBaseline = false;
		Reset();
	}

	void ThreadProfiler::BindMainThread()
	{
		if (m_threadCount == 0) return;

		t_pActivity = &m_upActivities[0];
		t_pActivity->Transition(EThreadState::Busy, Now());
	}

	void ThreadProfiler::BindWorkerThread(uint32_t a_workerID)
	{
		if (a_workerID + 1 >= m_threadCount) return;

		t_pActivity = &m_upActivities[a_workerID + 1];
		t_pActivity->Transition(EThreadState::Idle, Now());
	}

	void ThreadProfiler::UnbindCurrentThread()
	{
		t_pActivity = nullptr;
	}

	EThreadState ThreadProfiler::EnterState(EThreadState a_next)
	{
		ThreadActivity* _pActivity = t_pActivity;
		if (!_pActivity) return a_next;

		return _pActivity->Transition(a_next, Now());
	}

	void ThreadProfiler::CountJob()
	{
		ThreadActivity* _pActivity = t_pActivity;
		if (!_pActivity) return;

		// 書くのは自分だけなので、読み書きを分けてよい
		_pActivity->jobCount.store(_pActivity->jobCount.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
	}

	void ThreadProfiler::CountSteal()
	{
		ThreadActivity* _pActivity = t_pActivity;
		if (!_pActivity) return;

		_pActivity->stealCount.store(_pActivity->stealCount.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
	}

	//======================================================================================
	// フレームの区切り
	//
	// 各スレッドの記録は起動からの合計しか持っていないので、
	// 前回読んだ値との差分をこのフレームぶんとして足し込む
	//======================================================================================
	void ThreadProfiler::EndFrame()
	{
		if (m_threadCount == 0) return;

		const int64_t _now = Now();

		// 初回は起点を取るだけ : 起動からここまでをフレームとして数えない
		const bool _isAccumulate = m_hasBaseline;
		if (_isAccumulate)
		{
			m_sumFrameTicks += _now - m_lastFrameTicks;
		}
		m_lastFrameTicks = _now;
		m_hasBaseline = true;

		for (uint32_t _i = 0; _i < m_threadCount; ++_i)
		{
			const ThreadActivity& _activity = m_upActivities[_i];
			ThreadAccumulator& _accum = m_accumulators[_i];

			int64_t _ticks[STATE_COUNT] = {};
			_activity.Sample(_now, _ticks);

			const uint64_t _jobCount = _activity.jobCount.load(std::memory_order_relaxed);
			const uint64_t _stealCount = _activity.stealCount.load(std::memory_order_relaxed);

			if (_isAccumulate)
			{
				for (size_t _s = 0; _s < STATE_COUNT; ++_s)
				{
					_accum.sumTicks[_s] += _ticks[_s] - _accum.lastTicks[_s];
				}

				const int64_t _busyTicks =
					_ticks[static_cast<size_t>(EThreadState::Busy)] - _accum.lastTicks[static_cast<size_t>(EThreadState::Busy)];
				_accum.maxBusyTicks = (std::max)(_accum.maxBusyTicks, _busyTicks);

				_accum.sumJobCount += _jobCount - _accum.lastJobCount;
				_accum.sumStealCount += _stealCount - _accum.lastStealCount;
			}

			std::copy(std::begin(_ticks), std::end(_ticks), std::begin(_accum.lastTicks));
			_accum.lastJobCount = _jobCount;
			_accum.lastStealCount = _stealCount;
		}

		if (!_isAccumulate) return;

		++m_frameCount;
		if (m_frameCount >= m_averageRate)
		{
			Publish();
			ClearAccumulation();
		}
	}

	void ThreadProfiler::Reset()
	{
		ClearAccumulation();
		m_snapshot = {};
	}

	void ThreadProfiler::Publish()
	{
		if (m_frameCount <= 0) return;

		const double _frameCount = static_cast<double>(m_frameCount);
		const double _sumFrameTicks = static_cast<double>(m_sumFrameTicks);

		m_snapshot.threads.resize(m_threadCount);
		m_snapshot.frameMs = TicksToMs(_sumFrameTicks / _frameCount);
		m_snapshot.sampleFrameCount = static_cast<uint32_t>(m_frameCount);
		++m_snapshot.publishCount;

		for (uint32_t _i = 0; _i < m_threadCount; ++_i)
		{
			const ThreadAccumulator& _accum = m_accumulators[_i];
			ThreadProfileStats& _stats = m_snapshot.threads[_i];

			const double _busyTicks = static_cast<double>(_accum.sumTicks[static_cast<size_t>(EThreadState::Busy)]);

			_stats.busyMs = TicksToMs(_busyTicks / _frameCount);
			_stats.jobWaitMs = TicksToMs(static_cast<double>(_accum.sumTicks[static_cast<size_t>(EThreadState::JobWait)]) / _frameCount);
			_stats.idleMs = TicksToMs(static_cast<double>(_accum.sumTicks[static_cast<size_t>(EThreadState::Idle)]) / _frameCount);
			_stats.maxBusyMs = TicksToMs(static_cast<double>(_accum.maxBusyTicks));
			_stats.utilization = (_sumFrameTicks > 0.0) ? (_busyTicks / _sumFrameTicks) : 0.0;

			_stats.jobCount = static_cast<double>(_accum.sumJobCount) / _frameCount;
			_stats.stealCount = static_cast<double>(_accum.sumStealCount) / _frameCount;
		}
	}

	void ThreadProfiler::ClearAccumulation()
	{
		for (auto& _accum : m_accumulators)
		{
			std::fill(std::begin(_accum.sumTicks), std::end(_accum.sumTicks), int64_t(0));
			_accum.maxBusyTicks = 0;
			_accum.sumJobCount = 0;
			_accum.sumStealCount = 0;
		}
		m_sumFrameTicks = 0;
		m_frameCount = 0;
	}
}
