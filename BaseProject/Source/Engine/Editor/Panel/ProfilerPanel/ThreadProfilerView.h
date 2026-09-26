#pragma once

namespace Engine::Thread
{
	class ThreadProfiler;
	struct ThreadProfileSnapshot;
}

namespace Engine::Editor
{
	/// <summary>
	/// スレッドごとの稼働時間を表示する(ProfilerPanel の Thread 表示)
	///
	/// ジョブシステムが回っているか・ワーカーへ均等に割り振れているかを見るためのもの。
	/// 計測は ThreadProfiler の担当なので、ここは確定済みの平均を並べるだけ
	/// </summary>
	class ThreadProfilerView
	{
	public:

		void Draw();

	private:

		// 全体の要約 : 並列度・ワーカー間の偏り・気になる点
		void DrawSummary(const Thread::ThreadProfileSnapshot& a_snapshot);

		// スレッドごとの表
		void DrawThreadTable(const Thread::ThreadProfileSnapshot& a_snapshot);
	};
}
