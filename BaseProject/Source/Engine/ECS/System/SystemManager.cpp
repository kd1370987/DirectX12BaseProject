#include "SystemManager.h"

#include "../World/Profile/ECSWorldProfiler.h"

#include "../../JobSystem/JobSystem.h"

namespace Engine::ECS
{

	namespace
	{
		// 同時に走らせると壊れる組み合わせ : 向きは問わない
		bool IsConflict(const SystemTask& a_lhs,const SystemTask& a_rhs)
		{
			return (a_lhs.writeSig & (a_rhs.readSig | a_rhs.writeSig)).any()	// 書く * 読む / 書く * 書く
				|| (a_lhs.readSig & a_rhs.writeSig).any();						// 読む * 書く
		}

		// システムの実行 : 同期・ジョブ関係なく
		void ExecuteTask(SystemTask& a_task, const SystemContext& a_context, double* a_pOutMs)
		{
			// 計測しないときは時計も読まない
			if (!a_pOutMs)
			{
				a_task.executeFunc(a_task, a_context);
				return;
			}

			const auto _begin = std::chrono::steady_clock::now();
			a_task.executeFunc(a_task, a_context);
			const auto _end = std::chrono::steady_clock::now();

			// 計測結果
			*a_pOutMs = std::chrono::duration<double, std::milli>(_end - _begin).count();
		}
	}

	void SystemManager::Hold(std::shared_ptr<ISystem> a_spSystem)
	{
		if (!a_spSystem) return;
		m_systemVec.push_back(std::move(a_spSystem));
	}

	void SystemManager::Init()
	{
		m_systemVec.clear();
	}

	void SystemManager::RunSystem(const ESystemType& a_type, const SystemContext& a_context, ECSWorldProfiler* a_pProfiler)
	{
		// フェーズ検索
		auto _cit = m_compiledTaskMap.find(a_type);
		if (_cit == m_compiledTaskMap.end()) return;

		// フェーズ内システムのチェック
		auto& _compiledVec = _cit->second;
		const uint32_t _taskCount = static_cast<uint32_t>(_compiledVec.size());
		if (_taskCount == 0) return;

		// ジョブシステムがない ・ 止まっているときは全部同期で回す
		Thread::JobSystem* _pJobSystem = a_context.pServices ? a_context.pServices->pJobSystem : nullptr;
		if (_pJobSystem && !_pJobSystem->IsRunning()) _pJobSystem = nullptr;

		const bool _isMeasure = (a_pProfiler != nullptr);

		// このフェーズように作り直す : ジョブが要素のアドレスを持つため、以下で resize しない
		m_jobScratch.assign(_taskCount,nullptr);
		m_taskMsScratch.assign(_taskCount, 0.0f);

		for (uint32_t _j = 0; _j < _taskCount; ++_j)
		{
			SystemTask* _pTask = _compiledVec[_j].pTask;
			double* _pOutMs = _isMeasure ? &m_taskMsScratch[_j] : nullptr;

			// 待つ相手を、今フレームのJob*に引き直す
			// nullptr == 同期で走った・積めなかった → もう終わっているので待たない
			m_depScratch.clear();
			for (uint32_t _i : _compiledVec[_j].waitIndices)
			{
				if (m_jobScratch[_i]) m_depScratch.push_back(m_jobScratch[_i]);
			}

			// Jobタスク : 待つ相手の後続に積む メインスレッドは止まらない
			if (_pTask->exec == ETaskExec::Job && _pJobSystem)
			{
				m_jobScratch[_j] = _pJobSystem->PushJob(
					[_pTask,_context = a_context,_pOutMs]()
					{
						ExecuteTask(*_pTask,_context,_pOutMs);
					},
					m_depScratch
				);

				// 積めなかった → 下の同期実行へ落とす。待つ相手は m_depScracthにいる
				if (m_jobScratch[_j]) continue;
			}

			// 同期タスク : ぶつかるジョブが終わるのを直前で待つ
			for (Thread::Job* _pDep : m_depScratch)
			{
				_pJobSystem->WaitFor(_pDep);
			}

			ExecuteTask(*_pTask, a_context, _pOutMs);
		}

		// フェーズの終わりの待ち合わせ
		// フェーズの間には物理の更新やBeginFrameの構造変更が入るので持ち越さない
		if (_pJobSystem)
		{
			for (Thread::Job* _pJob : m_jobScratch)
			{
				if (_pJob) _pJobSystem->WaitFor(_pJob);
			}
		}

		// 計測の反応はメインスレッドで全部終わってから行う
		if (_isMeasure)
		{
			for (uint32_t _i = 0; _i < _taskCount; ++_i)
			{
				a_pProfiler->RecordTaskTime(_compiledVec[_i].pTask,m_taskMsScratch[_i]);
			}
		}

		// Job* をフェーズの外へ持ち出さない
		m_jobScratch.clear();
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

			const bool _isSuccess = Engine::Algorithm::Graph::TopologicalSort(
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

			// 待つ相手の組み立て
			auto& _compiledVec = m_compiledTaskMap[_systemPhase];
			_compiledVec.clear();					// Sortはタスクが増えるたびに走りなおす必要があるので空にする
			_compiledVec.resize(_sortedVec.size());

			for (uint32_t _j = 0; _j < _sortedVec.size(); ++_j)
			{
				CompileTask& _compiled = _compiledVec[_j];
				_compiled.pTask = _sortedVec[_j];

				// 自身より前のシステムを基準として判断
				for (uint32_t _i = 0; _i < _j; ++_i)
				{
					const SystemTask* _pPrev = _sortedVec[_i];

					// 前にある同期タスクは、自分の番が来た時点ですでに終わっている想定
					if (_pPrev->exec != ETaskExec::Job) continue;

					// Jobのみ待つのかどうか判断
					if (IsConflict(*_pPrev, *_compiled.pTask))
					{
						_compiled.waitIndices.push_back(_i);
					}
				}
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
