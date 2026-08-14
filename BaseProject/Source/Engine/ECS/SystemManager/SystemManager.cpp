#include "SystemManager.h"

namespace Engine::ECS
{

	void SystemManager::Init()
	{
		m_systemVec.clear();
	}

	void SystemManager::RunSystem(const ESystemType& a_type, const SystemContext& a_context)
	{

		// フェーズ検索
		auto _cit = m_compileTaskMap.find(a_type);
		if (_cit != m_compileTaskMap.end())
		{
			// フェーズ内のソートされたシステムを順に回す
			for (auto& _task : _cit->second)
			{
				_task->executeFunc(a_context);
			}
		}
	}

	void SystemManager::Sort()
	{
		// 変更がなければソートしない
		if (!m_isChange) return;

		// ソート対象を生ポインタへ並べ直すための入れ物 : フェーズごとに使い回す
		std::vector<SystemTask*> _taskVec = {};

		// システムフェーズごとに配列をＤＡＧグラフにする
		for (auto& [_systemPhase, _systemTaskVec] : m_systemTaskMap)
		{
			_taskVec.clear();
			_taskVec.reserve(_systemTaskVec.size());
			for (auto& _upTask : _systemTaskVec)
			{
				_taskVec.push_back(_upTask.get());
			}

			Algorithm::Graph::TopologicalSort(
				_taskVec,
				m_compileTaskMap[_systemPhase],
				[](const SystemTask* a, const SystemTask* b)
				{
					// ビット演算で論理積をとり一つでも立っていたらtrue
					return (a->readSig & b->writeSig).any();
				}
			);
		}

		// 組み直しが済んだので、次にタスクが増えるまでは何もしない。
		// 増えたときは AddSystemTask がフラグを立て直すので、
		// 次の BeginFrame で必ずここを通る
		m_isChange = false;
	}

	void SystemManager::AddSystemTask(ESystemType a_systemType, const SystemTask & a_systemTask, const std::string& a_taskName)
	{
		m_isChange = true;

		auto _upTask = std::make_unique<SystemTask>(a_systemTask);

		// 名前が入っていなければ登録時の名前を使う
		if (_upTask->name.empty())
		{
			_upTask->name = a_taskName;
		}

		m_systemTaskMap[a_systemType].push_back(std::move(_upTask));
	}

	const std::unordered_map<ESystemType, std::vector<SystemTask*>>& SystemManager::GetCompileTaskMap() const
	{
		return m_compileTaskMap;
	}
}
