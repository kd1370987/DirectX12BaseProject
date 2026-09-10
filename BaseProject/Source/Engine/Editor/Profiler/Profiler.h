#pragma once
namespace Engine::Editor
{
	/// <summary>
	/// 計測名1件ぶんの集計結果
	///
	/// 計測側(Engine::Debug::TimeProfileScope)が投げてくるのは
	/// 「名前」と「1回ぶんの時間」だけなので、
	/// 平均・最小・最大・呼び出し回数はすべてここで組み立てる
	///
	/// すべて MS (ミリ秒) での計測を前提
	/// </summary>
	struct ScopeTimer
	{
		//---------------------------------------------------------------------
		// 直近フレームの結果 : EndFrameで確定する
		//---------------------------------------------------------------------
		double time = 0.0;					// 直近フレームで使った合計時間
		int    callCount = 0;				// 直近フレームで通った回数

		//---------------------------------------------------------------------
		// 集計結果
		//---------------------------------------------------------------------
		double averageTime = 0.0;			// 平均時間 (平均レートごとに更新)
		double minTime = 0.0;				// リセット以降の最小時間 (1フレームの合計で見る)
		double maxTime = 0.0;				// リセット以降の最大時間 (1フレームの合計で見る)

		double accumulatedTime = 0.0;		// 平均計算用の累積
		int    sampleFrameCount = 0;		// 平均計算用のフレーム数 (平均確定時に0へ戻る)

		int    totalCallCount = 0;			// リセット以降の総計測回数
		bool   hasSample = false;			// 一度でも計測できたか (min/maxの初期化判定)

		//---------------------------------------------------------------------
		// 集計中のフレーム
		//
		// コールバックは1フレームに何度でも飛んでくるので、
		// いったんここへ足しておき、EndFrameで time / callCount へ移す
		//---------------------------------------------------------------------
		double pendingTime = 0.0;
		int    pendingCallCount = 0;
	};

	/// <summary>
	/// 表示用の計測結果
	/// パネルはこの配列を上から並べるだけでよい
	/// </summary>
	struct ScopeTimerResult
	{
		std::string name;					// 計測名 (ENGINE_PROFILE_SCOPEに渡した文字列)
		ScopeTimer  timer;					// 集計結果のスナップショット
	};

	/// <summary>
	/// ENGINE_PROFILE_SCOPE の結果を受け取って集計するクラス
	///
	/// 計測側はコールバックへ結果を投げるだけで、集計のことを何も知らない。
	/// 平均や最小最大が欲しいのは表示するエディター側なので、ここで組み立てる。
	/// 集計・並べ替えはすべてここが持ち、パネルは GetResults() を読むだけにする
	/// </summary>
	/// <remarks>
	/// PushResult はワーカースレッドから呼ばれることがある。
	/// なので受け取りは「受け取り待ちへ積むだけ」にして、
	/// 集計はフレーム末尾(EndFrame)にメインスレッドでまとめて行う
	/// </remarks>
	class Profiler
	{
	public:

		/// <summary>
		/// 計測結果を1件受け取る (別スレッドから呼ばれる)
		/// </summary>
		void PushResult(const Debug::ProfileResult& a_result);

		/// <summary>
		/// フレーム末尾の集計
		/// 受け取り待ちを掃き出し、平均レートに達していれば平均を確定させる
		/// </summary>
		/// <remarks>
		/// 受け取り待ちを掃き出すのはここだけなので、必ず毎フレーム通すこと
		/// </remarks>
		void EndFrame();

		// 集計結果を全部初期化する
		void ResetAll();

		// アクセサ
		void SetAvelageRate(int a_rate) { m_avelageRate = (a_rate > 0) ? a_rate : 1; }
		int GetAvelageRate() const { return m_avelageRate; }

		/// <summary>
		/// 平均時間の降順に並べ替え済みの計測結果を取得する
		/// 中身は EndFrame の時点のスナップショットなので、描画中に増減しない
		/// </summary>
		const std::vector<ScopeTimerResult>& GetResults() const { return m_results; }

	private:

		// 受け取り待ち : PushResult が積み、EndFrame が掃き出す
		std::mutex						  m_pendingMutex;
		std::vector<Debug::ProfileResult> m_pendingResults = {};

		// 名前ごとの集計
		std::unordered_map<std::string, ScopeTimer> m_timers = {};

		// 表示用に並べ替えた結果 : EndFrameで作り直す
		std::vector<ScopeTimerResult> m_results = {};

		// 平均を求める際の分割レート
		int m_avelageRate = 60;
		int m_frameCount = 0;
	};
}
