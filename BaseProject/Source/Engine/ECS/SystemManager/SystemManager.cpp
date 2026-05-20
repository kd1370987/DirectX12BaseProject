#include "SystemManager.h"

namespace Engine::ECS
{

	void SystemManager::Init()
	{
		for (UINT _i = 0; _i < (UINT)ESystemType::Num; ++_i)
		{
			m_systemMap.emplace((ESystemType)_i, std::vector<std::shared_ptr<ISystem>>{});
		}
	}

	void SystemManager::RunSystem(World& a_world, const ESystemType& a_type, float a_dt)
	{

		// フェーズ検索
		auto _cit = m_compileTaskMap.find(a_type);
		if (_cit != m_compileTaskMap.end())
		{
			// フェーズ内のソートされたシステムを順に回す
			for (auto& _task : _cit->second)
			{
				_task->executeFunc(a_dt);
			}
		}
	}

	void SystemManager::Sort()
	{
		// 変更がなければソートしない
		if (!m_isChange) return;

		// システムフェーズごとに配列をＤＡＧグラフにする
		for (auto& [_systemPhase, _systemTaskVec] : m_systemTaskMap)
		{
			auto& _dstTaskVec = m_compileTaskMap[_systemPhase];
			Algorithm::Graph::TopologicalSort(
				_systemTaskVec,
				_dstTaskVec,
				[&](auto& a, auto& b)
				{
					// ビット演算で論理積をとり一つでも立っていたらtrue
					return (a.readSig & b.writeSig).any();
				}
			);
		}
	}

	void SystemManager::AddSystemTask(ESystemType a_systemType, const SystemTask & a_systemTask)
	{
		m_isChange = true;

		auto _it = m_systemTaskMap.find(a_systemType);
		if (_it != m_systemTaskMap.end())
		{
			_it->second.push_back(a_systemTask);
		}
		else
		{
			m_systemTaskMap[a_systemType] = {};
			m_systemTaskMap[a_systemType].push_back(a_systemTask);
		}
	}
}