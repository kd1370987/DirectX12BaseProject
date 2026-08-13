#pragma once
namespace Engine::Thread
{
	class JobWorker;

	struct JobContext
	{
		std::vector<JobWorker*> pJobWorker = {};
	};
}