#pragma once
namespace Engine::Thread
{
	/// <summary>
	/// ジョブシステムに任せる際の処理の入れ物
	/// </summary>
	struct Job
	{
		Handle<Job> handle;					// 自身のハンドル
		std::function<void()> task = {};	// 処理の実体

		// 自身の完了によって実行可能になるJob
		std::array<Handle<Job>, 8> dependents;
		std::atomic<uint32_t> dependentCount = 0;

		// 実行可能になるまでに残っている依存数
		std::atomic<uint32_t> unresolvedDependencies = 0;
	};
}