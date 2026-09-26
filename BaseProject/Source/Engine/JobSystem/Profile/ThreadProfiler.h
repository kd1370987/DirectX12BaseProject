#pragma once

namespace Engine::Thread
{
	//==========================================================================================
	// スレッドが今なにをしているか
	//
	// 「フレーム中に動いた時間」は Busy の合計で見る。
	// 止まっている時間を理由ごとに分けておくのは、
	//   JobWait が長い -> ジョブの依存や粒度のせいでメインが待たされている
	//   Idle が長い    -> そもそも仕事が回ってきていない
	// を見分けるため
	//==========================================================================================
	enum class EThreadState : uint8_t
	{
		Idle,		// 仕事がなくて止まっている : ワーカーは仕事待ちで寝ている / メインは GPU・垂直同期・FPS制限で止まっている
		Busy,		// 処理している : ワーカーはジョブ実行中 / メインは通常の処理
		JobWait,	// ジョブの完了待ちで止まっている (WaitFor / WaitForAll)

		Count,
	};

	/// <summary>
	/// スレッド1本ぶんの平均 (平均レートのフレーム数で割ったもの)
	/// 時間はすべて1フレームあたりのミリ秒
	/// </summary>
	struct ThreadProfileStats
	{
		double busyMs = 0.0;			// 動いていた時間
		double jobWaitMs = 0.0;			// ジョブの完了待ちで止まっていた時間
		double idleMs = 0.0;			// 仕事がなくて止まっていた時間
		double maxBusyMs = 0.0;			// 区間内で一番動いていたフレームの値
		double utilization = 0.0;		// 動いていた割合 (0 ~ 1) : busy / フレーム時間

		double jobCount = 0.0;			// 実行したジョブ数 (盗んだ分も含む)
		double stealCount = 0.0;		// ほかのワーカーから盗んで実行したジョブ数
	};

	/// <summary>
	/// 表示用の集計結果 : 平均レートごとに作り直す
	/// </summary>
	struct ThreadProfileSnapshot
	{
		// 先頭がメインスレッド、以降がワーカー(ワーカーID順)
		std::vector<ThreadProfileStats> threads = {};

		double frameMs = 0.0;			// 平均フレーム時間
		uint32_t sampleFrameCount = 0;	// 平均に使ったフレーム数
		uint32_t publishCount = 0;		// 確定した回数 : 0 ならまだ一度も集計できていない
	};

	// スレッド1本ぶんの記録 : 中身は cpp だけが知っていればよい
	struct ThreadActivity;

	/// <summary>
	/// メインスレッドと各ワーカースレッドが、フレーム中にどれだけ動いたかを測る
	///
	/// ジョブシステムが正常に回っているか、ワーカーへ均等に割り振れているかを見るためのもの。
	///
	/// 各スレッドは「状態が切り替わった瞬間」に経過時間を自分の記録へ足すだけで、
	/// フレームの区切りや平均の計算はすべてメインスレッドの EndFrame が行う。
	/// 状態の切り替えは JobWorker(ジョブの実行)・JobSystem(完了待ち)・
	/// 描画や時間管理の待機箇所(ThreadStateScope)から行われる
	/// </summary>
	/// <remarks>
	/// 長いジョブ(アセットのロードなど)がフレームを跨いでも、
	/// EndFrame は「実行中の区間をその時点までで区切って」読むので、
	/// 終わったフレームにまとめて乗ることはない
	/// </remarks>
	class ThreadProfiler
	{
	public:

		ThreadProfiler();
		~ThreadProfiler();
		NON_COPYABLE_NON_MOVABLE(ThreadProfiler);

		/// <summary>
		/// 計測用の記録を用意する : ワーカーを起動する前に呼ぶこと
		/// </summary>
		/// <param name="a_workerCount">ワーカースレッド数 (メインスレッドの分は内部で足す)</param>
		void Init(uint32_t a_workerCount);

		//------------------------------------------------------------------
		// スレッドの登録 : 呼んだスレッド自身を計測対象にする
		//------------------------------------------------------------------

		// メインスレッド : 普段は Busy で、待機箇所だけ止まっている扱いにする
		void BindMainThread();

		// ワーカースレッド : 普段は Idle で、ジョブを実行している間だけ Busy にする
		void BindWorkerThread(uint32_t a_workerID);

		// 登録を外す : 記録を破棄する前に、登録したスレッド自身で呼ぶこと
		static void UnbindCurrentThread();

		//------------------------------------------------------------------
		// 計測する側 (どのスレッドからでも呼べる。登録していないスレッドでは何もしない)
		//------------------------------------------------------------------

		/// <summary>
		/// 呼んだスレッドの状態を切り替える
		/// </summary>
		/// <returns>切り替える前の状態 : 戻すときに渡す</returns>
		static EThreadState EnterState(EThreadState a_next);

		// ジョブを1件実行したことを数える
		static void CountJob();

		// ほかのワーカーから盗んだことを数える
		static void CountSteal();

		//------------------------------------------------------------------
		// 集計 (メインスレッドのみ)
		//------------------------------------------------------------------

		/// <summary>
		/// フレームの区切り : 毎フレーム同じ位置で1回だけ呼ぶこと
		/// 前回呼んだときからの差分を1フレームぶんとして足し込み、
		/// 平均レートに達していれば平均を確定させる
		/// </summary>
		void EndFrame();

		// 集計結果を捨てて取り直す
		void Reset();

		// アクセサ
		void SetAverageRate(int a_rate) { m_averageRate = (a_rate > 0) ? a_rate : 1; }
		int GetAverageRate() const { return m_averageRate; }
		uint32_t GetWorkerCount() const { return m_threadCount > 0 ? m_threadCount - 1 : 0; }

		// 直近で確定した平均 : EndFrame と同じメインスレッドから読むこと
		const ThreadProfileSnapshot& GetSnapshot() const { return m_snapshot; }

	private:

		// スレッド1本ぶんの、前回の読み取り値と区間内の積算 : メインスレッドだけが触る
		struct ThreadAccumulator
		{
			int64_t lastTicks[static_cast<size_t>(EThreadState::Count)] = {};
			uint64_t lastJobCount = 0;
			uint64_t lastStealCount = 0;

			int64_t sumTicks[static_cast<size_t>(EThreadState::Count)] = {};
			int64_t maxBusyTicks = 0;
			uint64_t sumJobCount = 0;
			uint64_t sumStealCount = 0;
		};

		// 区間内の積算から平均を作って m_snapshot へ出す
		void Publish();

		// 区間内の積算だけを空にする (前回の読み取り値は残す)
		void ClearAccumulation();

	private:

		// スレッドごとの記録 : 先頭がメイン、以降がワーカー
		// 各スレッドが別々に書き込むので、キャッシュラインを分けて並べてある
		std::unique_ptr<ThreadActivity[]> m_upActivities = nullptr;
		uint32_t m_threadCount = 0;

		std::vector<ThreadAccumulator> m_accumulators = {};

		int64_t m_lastFrameTicks = 0;		// 前回 EndFrame を呼んだ時刻
		int64_t m_sumFrameTicks = 0;		// 区間内のフレーム時間の合計
		bool m_hasBaseline = false;			// 差分の起点を取ったか : 初回は起点を取るだけにする

		int m_averageRate = 60;				// 平均を取り直すフレーム数
		int m_frameCount = 0;				// 区間内のフレーム数

		ThreadProfileSnapshot m_snapshot = {};
	};

	/// <summary>
	/// スコープの間だけ、呼んだスレッドの状態を切り替える
	/// 抜けるときに元の状態へ戻すので、入れ子にしてよい
	///
	///   ワーカーのジョブ中に WaitFor -> Busy から JobWait へ、戻ると Busy
	///   メインの GPU 待ち            -> Busy から Idle へ、戻ると Busy
	/// </summary>
	class ThreadStateScope
	{
	public:
		explicit ThreadStateScope(EThreadState a_state)
			: m_prevState(ThreadProfiler::EnterState(a_state))
		{}
		~ThreadStateScope()
		{
			ThreadProfiler::EnterState(m_prevState);
		}
		NON_COPYABLE_NON_MOVABLE(ThreadStateScope);

	private:
		EThreadState m_prevState;
	};
}
