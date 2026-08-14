#pragma once
namespace Engine::Thread
{
	/// <summary>
	/// ジョブシステムに任せる際の処理の入れ物
	/// </summary>
	struct Job
	{
		Job() = default;
		Job(std::function<void()>&& a_task, Job* a_pParent = nullptr) : task(a_task), pParent(a_pParent) {}

		void Run() 
		{
			task();
			unfinishedJobs--;

			if (Finished())
			{
				if (pParent)
				{
					pParent->unfinishedJobs--;
				}
			}
		};

		bool Finished()const { return unfinishedJobs == 0; }

		std::function<void()> task;

		// 自身が終わった際に通知するジョブ
		Job* pParent = nullptr;

		// 自身を走らせるために完了すべきジョブ数
		std::atomic<uint32_t> unfinishedJobs = 1;
	};
}