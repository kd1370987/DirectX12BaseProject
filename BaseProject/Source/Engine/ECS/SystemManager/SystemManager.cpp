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

			auto& _sortedVec = m_compileTaskMap[_systemPhase];

			const bool _isSuccess = Algorithm::Graph::TopologicalSort(
				_taskVec,
				_sortedVec,
				[](const SystemTask* a, const SystemTask* b)
				{
					// ビット演算で論理積をとり一つでも立っていたらtrue
					return (a->readSig & b->writeSig).any();
				}
			);

			// 失敗＝依存が循環している。
			// ソート結果には循環に巻き込まれたタスクが入らないので、
			// そのまま使うとシステムが黙って実行されなくなる。
			// 何が落ちたのかを出したうえで、登録順で後ろに足して実行だけは続けさせる。
			if (!_isSuccess)
			{
				ReportSortFailure(_systemPhase, _taskVec, _sortedVec);
			}
		}

		// 組み直しが済んだので、次にタスクが増えるまでは何もしない。
		// 増えたときは AddSystemTask がフラグを立て直すので、
		// 次の BeginFrame で必ずここを通る
		m_isChange = false;
	}

	void SystemManager::ReportSortFailure(
		ESystemType a_phase,
		const std::vector<SystemTask*>& a_allTaskVec,
		std::vector<SystemTask*>& a_sortedTaskVec)
	{
		ENGINE_LOG("[ECS] システムのトポロジカルソートに失敗しました (phase = %d)", static_cast<int>(a_phase));
		ENGINE_LOG("[ECS] 依存が循環しています。下記のタスクの read/write を見直してください");

		// 並べられなかった＝循環に巻き込まれたタスク
		for (SystemTask* _pTask : a_allTaskVec)
		{
			if (!_pTask) continue;

			const bool _isSorted =
				std::find(a_sortedTaskVec.begin(), a_sortedTaskVec.end(), _pTask) != a_sortedTaskVec.end();
			if (_isSorted) continue;

			ENGINE_LOG("[ECS]   循環: %s", _pTask->name.c_str());

			// 相手も出す。read と write が互いに噛み合っているものが原因
			for (SystemTask* _pOther : a_allTaskVec)
			{
				if (!_pOther || _pOther == _pTask) continue;

				const bool _isMutual =
					(_pTask->readSig & _pOther->writeSig).any() &&
					(_pOther->readSig & _pTask->writeSig).any();

				if (_isMutual)
				{
					ENGINE_LOG("[ECS]     <-> %s (相互に read/write が噛み合っています)", _pOther->name.c_str());
				}
			}

			// 実行だけは続けさせる(登録順で末尾に足す)
			a_sortedTaskVec.push_back(_pTask);
		}
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
