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

		// 処理を回して、所要時間を a_pOutNs へ足し込む : 同期・ジョブ関係なく
		// 分割したタスクは複数のワーカーから同じ置き場へ足すので、代入ではなく加算にする。
		// 読むのは全ジョブを待ち終えたメインスレッドなので、順序は WaitFor 側で揃っている
		template<typename Func>
		void Measure(std::atomic<int64_t>* a_pOutNs, Func&& a_func)
		{
			// 計測しないときは時計も読まない
			if (!a_pOutNs)
			{
				a_func();
				return;
			}

			const auto _begin = std::chrono::steady_clock::now();
			a_func();
			const auto _end = std::chrono::steady_clock::now();

			a_pOutNs->fetch_add(
				std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _begin).count(),
				std::memory_order_relaxed);
		}

		// タスクを丸ごと1回実行する
		void ExecuteTask(SystemTask& a_task, const SystemContext& a_context, std::atomic<int64_t>* a_pOutNs)
		{
			Measure(a_pOutNs, [&]() { a_task.executeFunc(a_task, a_context); });
		}

		// タスクのチャンク [begin, end) を実行する
		void ExecuteTaskRange(SystemTask& a_task, const SystemContext& a_context, uint32_t a_begin, uint32_t a_end, std::atomic<int64_t>* a_pOutNs)
		{
			Measure(a_pOutNs, [&]() { a_task.executeRangeFunc(a_task, a_context, a_begin, a_end); });
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

		// このフェーズ用に作り直す : ジョブが要素のアドレスを持つため、以下で resize しない
		m_jobScratch.assign(_taskCount,nullptr);

		// 計測の置き場 : ここより下ではジョブが要素のアドレスを持つので作り直さない
		if (_isMeasure)
		{
			if (m_taskNsCapacity < _taskCount)
			{
				m_upTaskNsScratch = std::make_unique<std::atomic<int64_t>[]>(_taskCount);
				m_taskNsCapacity = _taskCount;
			}
			for (uint32_t _i = 0; _i < _taskCount; ++_i)
			{
				m_upTaskNsScratch[_i].store(0, std::memory_order_relaxed);
			}
		}

		for (uint32_t _j = 0; _j < _taskCount; ++_j)
		{
			SystemTask* _pTask = _compiledVec[_j].pTask;
			std::atomic<int64_t>* _pOutNs = _isMeasure ? &m_upTaskNsScratch[_j] : nullptr;

			// 待つ相手を、今フレームのJob*に引き直す
			// nullptr == 同期で走った・積めなかった → もう終わっているので待たない
			m_depScratch.clear();
			for (uint32_t _i : _compiledVec[_j].waitIndices)
			{
				if (m_jobScratch[_i]) m_depScratch.push_back(m_jobScratch[_i]);
			}

			// 待つ相手が終わるまでメインスレッドで待つ : 同期で回すときに使う
			auto _waitDependencies = [&]()
				{
					// 待つ相手があるなら _pJobSystem は必ず非 null
					for (Thread::Job* _pDep : m_depScratch)
					{
						_pJobSystem->WaitFor(_pDep);
					}
					m_depScratch.clear();
				};

			// Jobタスク : 待つ相手の後続に積む メインスレッドは止まらない
			if (_pTask->exec == ETaskExec::Job && _pJobSystem)
			{
				// 通常のタスク : チャンクを分けて複数のジョブで回す
				if (_pTask->executeRangeFunc)
				{
					// チャンク一覧はメインスレッドで確定させる
					const uint32_t _chunkNum = _pTask->prepareFunc(*_pTask, a_context);
					if (_chunkNum == 0) continue;

					const uint32_t _batchNum = std::min(_chunkNum, _pJobSystem->GetWorkerCount());
					const uint32_t _per = (_chunkNum + _batchNum - 1) / _batchNum;

					m_batchScratch.clear();
					for (uint32_t _b = 0; _b < _chunkNum; _b += _per)
					{
						const uint32_t _e = std::min(_b + _per, _chunkNum);

						Thread::Job* _pBatch = _pJobSystem->PushJob(
							[_pTask, _context = a_context, _b, _e, _pOutNs]()
							{
								ExecuteTaskRange(*_pTask, _context, _b, _e, _pOutNs);
							},
							m_depScratch			// 各バッチが待つ相手の後続になる
						);

						if (_pBatch)
						{
							m_batchScratch.push_back(_pBatch);
							continue;
						}

						// 積めなかった(ジョブシステムが止まった) :
						// フェンスは nullptr を無視して完了扱いにするので、黙って飛ばされないよう
						// 待つ相手を待ってからこの範囲をその場で回す
						_waitDependencies();
						ExecuteTaskRange(*_pTask, a_context, _b, _e, _pOutNs);
					}

					// 後ろのタスクはバッチごとではなくこのフェンスを待つようにする
					if (m_batchScratch.empty())
					{
						// 全部その場で回した
						m_jobScratch[_j] = nullptr;
					}
					else if (m_batchScratch.size() == 1)
					{
						m_jobScratch[_j] = m_batchScratch[0];
					}
					else
					{
						m_jobScratch[_j] = _pJobSystem->PushJob([] {}, m_batchScratch);

						// フェンスを積めなかった : 後ろから待てないので、ここで全バッチを待ち切る
						if (!m_jobScratch[_j])
						{
							for (Thread::Job* _pBatch : m_batchScratch)
							{
								_pJobSystem->WaitFor(_pBatch);
							}
						}
					}
					continue;
				}

				// カスタムタスク : 分けられないので1ジョブで積む
				m_jobScratch[_j] = _pJobSystem->PushJob(
					[_pTask, _context = a_context, _pOutNs]() { ExecuteTask(*_pTask, _context, _pOutNs); },
					m_depScratch);

				// 積めなかった → 下の同期実行へ
				if (m_jobScratch[_j]) continue;
			}

			// 同期タスク : ぶつかるジョブが終わるのを直前で待つ
			_waitDependencies();

			ExecuteTask(*_pTask, a_context, _pOutNs);
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

		// 計測の反映はメインスレッドで全部終わってから行う。
		// 分割したタスクは各バッチの時間の合計(CPU時間)になるので、実際の経過時間より長く出る
		if (_isMeasure)
		{
			for (uint32_t _i = 0; _i < _taskCount; ++_i)
			{
				const int64_t _ns = m_upTaskNsScratch[_i].load(std::memory_order_relaxed);
				a_pProfiler->RecordTaskTime(_compiledVec[_i].pTask, static_cast<double>(_ns) / 1'000'000.0);
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

			// ここまでに並べられた数(失敗したときは、ここから後ろが循環に巻き込まれたもの)
			const size_t _sortedCount = _sortedVec.size();

			// 失敗＝依存が循環している。
			// ソート結果には循環に巻き込まれたタスクが入らないので、
			// そのまま使うとシステムが黙って実行されなくなる。
			// 何が落ちたのかを出したうえで、登録順で後ろに足して実行だけは続けさせる。
			if (!_isSuccess)
			{
				ReportSortFailure(_systemPhase, _taskVec, _sortedVec);
			}

			// 並びの診断(実行には影響しない)
			BuildScheduleReport(_systemPhase, _taskVec, _sortedVec, _sortedCount);

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



	//----------------------------------------------------------------------------------------------
	// 並びの診断
	//
	// ソートの辺は RAW(自分が読むものを相手が書く → 相手の後)だけなので、
	// 「衝突はしているが RAW の経路で前後がつながっていない」組は、
	// Kahn法の段と登録順でたまたまその並びになっているだけになる。
	// 登録の位置やシステムの追加で黙って入れ替わりうるので、ここで拾って見えるようにする。
	//
	// 拾えるのは宣言(read / write)に出ているものだけ。
	// RefData やリソース越しの読み書きは宣言に出ないので、ここには現れない
	//----------------------------------------------------------------------------------------------
	void SystemManager::BuildScheduleReport(
		ESystemType a_phase,
		const std::vector<SystemTask*>& a_allTaskVec,
		const std::vector<SystemTask*>& a_sortedTaskVec,
		size_t a_sortedCount)
	{
		PhaseScheduleReport& _report = m_scheduleReportMap[a_phase];
		_report = {};

		const size_t _num = a_allTaskVec.size();
		_report.isSorted = (a_sortedCount >= _num);

		// 循環に巻き込まれ、末尾に足されたもの
		for (size_t _i = a_sortedCount; _i < a_sortedTaskVec.size(); ++_i)
		{
			_report.cyclicTaskVec.push_back(a_sortedTaskVec[_i]);
		}

		//------------------------------------------------------------------
		// RAW の経路で届くか(推移閉包)
		//   _reach[a][b] : a が終わってから b が走ることが依存で保証されている
		// フェーズあたりのタスクは数十なので、素直に辿ってよい
		//------------------------------------------------------------------
		std::vector<std::vector<uint8_t>> _reach(_num, std::vector<uint8_t>(_num, 0));
		for (size_t _from = 0; _from < _num; ++_from)
		{
			std::vector<size_t> _stack = { _from };
			while (!_stack.empty())
			{
				const size_t _cur = _stack.back();
				_stack.pop_back();

				for (size_t _to = 0; _to < _num; ++_to)
				{
					if (_to == _cur || _reach[_from][_to]) continue;

					// _to が読むものを _cur が書く → _cur の後に _to
					if ((a_allTaskVec[_to]->readSig & a_allTaskVec[_cur]->writeSig).none()) continue;

					_reach[_from][_to] = 1;
					_stack.push_back(_to);
				}
			}
		}

		//------------------------------------------------------------------
		// 今の並びで前後にある衝突の組のうち、経路でつながっていないもの
		//------------------------------------------------------------------
		// 登録順の添え字(_reach の添え字)を引けるようにしておく
		std::unordered_map<const SystemTask*, size_t> _indexMap = {};
		for (size_t _i = 0; _i < _num; ++_i)
		{
			_indexMap[a_allTaskVec[_i]] = _i;
		}

		for (size_t _e = 0; _e < a_sortedTaskVec.size(); ++_e)
		{
			const SystemTask* _pEarlier = a_sortedTaskVec[_e];
			const size_t _ei = _indexMap[_pEarlier];

			for (size_t _l = _e + 1; _l < a_sortedTaskVec.size(); ++_l)
			{
				const SystemTask* _pLater = a_sortedTaskVec[_l];
				if (!IsConflict(*_pEarlier, *_pLater)) continue;

				const size_t _li = _indexMap[_pLater];

				// どちらかの向きに経路があれば前後は依存で決まっている。
				// 両向きにある(循環の中)ものは循環側で報告しているので数えない
				if (_reach[_ei][_li] || _reach[_li][_ei]) continue;

				ScheduleAmbiguity& _amb = _report.ambiguityVec.emplace_back();
				_amb.pEarlier = _pEarlier;
				_amb.pLater = _pLater;
				_amb.conflictSig =
					(_pEarlier->writeSig & (_pLater->readSig | _pLater->writeSig)) |
					(_pEarlier->readSig & _pLater->writeSig);
			}
		}

		// 件数だけログへ出す(中身は ECS プロファイラの Systems で見る)
		if (!_report.isSorted || !_report.ambiguityVec.empty())
		{
			ENGINE_LOG("[ECS] %s : ソート%s / 循環 %zu 件 / 前後が依存で決まっていない衝突 %zu 組",
				magic_enum::enum_name(a_phase).data(),
				_report.isSorted ? "成功" : "失敗",
				_report.cyclicTaskVec.size(),
				_report.ambiguityVec.size());
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
