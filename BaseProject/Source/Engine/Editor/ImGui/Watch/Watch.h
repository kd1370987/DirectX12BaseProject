#pragma once

/// <summary>
/// クラス単体で一つを計測する
/// </summary>

// 表示する際の単位
enum class TimeUnit
{
	ns,		// ナノ秒
	us,		// マイクロ秒
	ms,		// ミリ秒
	s,		// 秒
	all,	// 全部
};

class Watch
{
public:

	/// <summary>
	/// 計測開始
	/// </summary>
	void Start();
	
	/// <summary>
	/// 計測終了
	/// </summary>
	void End();

	/// <summary>
	/// 計測結果を表示
	/// </summary>
	/// <param name="a_name">項目名</param>
	void DrawResult(
		const std::string& a_name,
		double a_watchTime = 0.0f,
		TimeUnit a_unit = TimeUnit::all
	);

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

private:

	using Clock = std::chrono::high_resolution_clock;
	
	// 平均をとる際の計測時間
	double m_watchTime = 0.0;
	int m_count = 0;


	// 累積時間
	double m_matchTime = 0.0;

	// 計測用
	Clock::time_point m_startTime;		// 開始時間
	Clock::time_point m_endTime;		// 終了時間

	double m_calcTime = 0.0;		// 一時結果

	double m_minTime = 100000.0;
	double m_maxTime = 0.0;

};