#include "Profiler.h"

namespace Engine::Editor
{
	//======================================================================================
	// 計測結果の受け取り
	//
	// ここはワーカースレッドから呼ばれることがあるので、積むだけにして何も触らない
	// (ログパネルと同じ考え方。集計はメインスレッドのEndFrameでやる)
	//======================================================================================
	void Profiler::PushResult(const Debug::ProfileResult& a_result)
	{
		std::lock_guard<std::mutex> _lock(m_pendingMutex);
		m_pendingResults.push_back(a_result);
	}

	//======================================================================================
	// フレーム末尾の集計
	//
	// 1. 受け取り待ちを掃き出して、名前ごとにこのフレームぶんを足す
	// 2. フレームの合計を確定させ、最小・最大を更新する
	// 3. 平均レートに達していたら平均を取り直す
	// 4. 表示用の並べ替え済み配列を作り直す
	//
	// パネルが読むのは常に「前フレームまでに確定した結果」になる
	//======================================================================================
	void Profiler::EndFrame()
	{
		//-----------------------------------------------------------------
		// 1. 受け取り待ちを掃き出す
		//
		// ロックしている間に集計まで済ませると、計測側(別スレッド)を
		// そのぶん待たせることになるので、入れ物ごと持ち出してすぐ手を放す
		//-----------------------------------------------------------------
		std::vector<Debug::ProfileResult> _results;
		{
			std::lock_guard<std::mutex> _lock(m_pendingMutex);
			_results.swap(m_pendingResults);
		}

		for (const auto& _result : _results)
		{
			// 未登録の名前はここで作る
			ScopeTimer& _timer = m_timers[_result.name];

			_timer.pendingTime += _result.ms;
			++_timer.pendingCallCount;
			++_timer.totalCallCount;
		}

		//-----------------------------------------------------------------
		// 2. このフレームぶんを確定させる
		//-----------------------------------------------------------------
		for (auto& [_name, _timer] : m_timers)
		{
			_timer.time = _timer.pendingTime;
			_timer.callCount = _timer.pendingCallCount;

			_timer.pendingTime = 0.0;
			_timer.pendingCallCount = 0;

			// このフレームで一度も通らなかったものは平均に混ぜない
			//
			// 混ぜてしまうと「たまにしか通らないが重い処理」の平均が
			// 通らなかったフレームの0で薄まって、軽く見えてしまう
			if (_timer.callCount <= 0) continue;

			_timer.accumulatedTime += _timer.time;
			++_timer.sampleFrameCount;

			// 最小・最大
			// 初回は比較対象がないのでそのまま入れる
			if (!_timer.hasSample)
			{
				_timer.hasSample = true;
				_timer.minTime = _timer.time;
				_timer.maxTime = _timer.time;
				_timer.averageTime = _timer.time;	// 平均が確定するまでの暫定値
			}
			else
			{
				_timer.minTime = std::min(_timer.minTime, _timer.time);
				_timer.maxTime = std::max(_timer.maxTime, _timer.time);
			}
		}

		//-----------------------------------------------------------------
		// 3. 平均の確定
		// 累積をフレーム数で割って平均に反映し、次の区間のために累積を空にする
		//-----------------------------------------------------------------
		++m_frameCount;
		if (m_frameCount >= m_avelageRate)
		{
			m_frameCount = 0;

			for (auto& [_name, _timer] : m_timers)
			{
				// この区間で一度も通らなかったものは、前回の平均を残す
				if (_timer.sampleFrameCount <= 0) continue;

				_timer.averageTime = _timer.accumulatedTime / static_cast<double>(_timer.sampleFrameCount);

				_timer.accumulatedTime = 0.0;
				_timer.sampleFrameCount = 0;
			}
		}

		//-----------------------------------------------------------------
		// 4. 表示用のスナップショットを作る
		//-----------------------------------------------------------------
		m_results.clear();
		m_results.reserve(m_timers.size());
		for (const auto& [_name, _timer] : m_timers)
		{
			m_results.push_back({ _name, _timer });
		}

		// 平均時間の降順(重い順)に並べ替え
		std::sort(
			m_results.begin(), m_results.end(),
			[](const ScopeTimerResult& a_lhs, const ScopeTimerResult& a_rhs)
			{
				return a_lhs.timer.averageTime > a_rhs.timer.averageTime;
			}
		);
	}

	//======================================================================================
	// リセット
	//
	// 名前ごと消してしまう
	// 計測箇所は毎フレーム名乗ってくるので、生きているものは次のフレームで戻ってくる
	//======================================================================================
	void Profiler::ResetAll()
	{
		{
			std::lock_guard<std::mutex> _lock(m_pendingMutex);
			m_pendingResults.clear();
		}

		m_timers.clear();
		m_results.clear();
		m_frameCount = 0;
	}
}
