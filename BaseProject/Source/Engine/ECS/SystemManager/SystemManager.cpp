#include "SystemManager.h"

#include "Engine/JobSystem/JobSystem.h"

namespace Engine::ECS
{

	void SystemManager::Init()
	{
		m_systemVec.clear();
	}

	void SystemManager::RunSystem(const ESystemType& a_type, const SystemContext& a_context)
	{
		// 並列で回せるフェーズは並列に流す。
		// ジョブシステムが使えない・並列対象外のフェーズなら直列へ落とす
		if (RunParallel(a_type, a_context)) return;

		RunSerial(a_type, a_context);
	}

	void SystemManager::Sort()
	{
		// 変更がなければソートしない
		if (!m_isChange) return;

		// システムフェーズごとに配列をＤＡＧグラフにする。
		// 走査中のマップを CompilePhase の中から引き直さないよう、
		// タスク列は参照で渡す
		for (auto& [_systemPhase, _systemTaskVec] : m_systemTaskMap)
		{
			CompilePhase(_systemPhase, _systemTaskVec);
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

	bool SystemManager::HasReadWriteDependency(const SystemTask& a_task, const SystemTask& a_other)
	{
		// 相手が書いたものを自分が読む : 書く側が先でなければ古い値を読む。
		// 逆向き(自分が書いて相手が読む)は、この関数を (相手, 自分) の順で
		// 呼んだときに引っかかるので、ここで見る必要はない
		return (a_task.readSig & a_other.writeSig).any();
	}

	bool SystemManager::HasWriteConflict(const SystemTask& a_task, const SystemTask& a_other)
	{
		// 同じ配列を2つのシステムが書くので、並列に走らせるとレースになる。
		// ただし「どちらが先か」は依存関係からは決まらない。
		// 向きは CompilePhase が読み書き依存で決めた実行順から取る
		return (a_task.writeSig & a_other.writeSig).any();
	}

	bool SystemManager::IsParallelPhase(ESystemType a_systemType)
	{
		// 描画フェーズは D3D12 のコマンドリストやエディタ(ImGui)を触るため
		// メインスレッド固定。
		// 初期化・解放系はエンティティの生成破棄そのものを行うので直列のまま置く
		switch (a_systemType)
		{
		case ESystemType::PreUpdate:
		case ESystemType::Update:
		case ESystemType::Physics:
		case ESystemType::Animation:
		case ESystemType::Camera:
		case ESystemType::PostUpdate:
			return true;

		default:
			return false;
		}
	}

	void SystemManager::CompilePhase(
		ESystemType a_systemType,
		const std::vector<std::unique_ptr<SystemTask>>& a_taskStorage
	)
	{
		auto& _dstTaskVec = m_compileTaskMap[a_systemType];
		auto& _dstGraphVec = m_compileGraphMap[a_systemType];

		_dstTaskVec.clear();
		_dstGraphVec.clear();

		const size_t _count = a_taskStorage.size();
		if (_count == 0) return;

		// 添え字で扱いたいので生ポインタへ並べ直す。
		// この並びが「登録順」になる
		std::vector<SystemTask*> _taskVec;
		_taskVec.reserve(_count);
		for (auto& _upTask : a_taskStorage)
		{
			_taskVec.push_back(_upTask.get());
		}

		//--------------------------------------------------------------------------------------
		// 1. 読み書きの依存だけで並べる
		//
		// 「書いた側が先、読む側が後」は依存関係から向きが一意に決まる。
		// 書き込み同士はここでは見ない : 向きが決まらないので、
		// 登録順などで無理に向きを付けると読み書きの依存と噛み合って循環しうる。
		// ここで出る並びは、これまでの直列実行の順序そのものになる
		//--------------------------------------------------------------------------------------
		std::vector<std::vector<uint32_t>> _successors(_count);		// 自分を待っている相手
		std::vector<uint32_t> _indegree(_count, 0);					// 自分が待つ数

		for (size_t _i = 0; _i < _count; ++_i)
		{
			for (size_t _j = 0; _j < _count; ++_j)
			{
				if (_i == _j) continue;

				if (!HasReadWriteDependency(*_taskVec[_i], *_taskVec[_j])) continue;

				_successors[_j].push_back(static_cast<uint32_t>(_i));
				++_indegree[_i];
			}
		}

		//--------------------------------------------------------------------------------------
		// トポロジカルソート
		// 待ち数が同時に 0 になったものは添え字の小さい順に出るので、
		// 並びの決め手が無いときは登録順が残る
		//--------------------------------------------------------------------------------------
		std::queue<uint32_t> _queue = {};
		for (size_t _i = 0; _i < _count; ++_i)
		{
			if (_indegree[_i] == 0) _queue.push(static_cast<uint32_t>(_i));
		}

		std::vector<uint32_t> _order = {};				// 実行順に並べた元の添え字
		_order.reserve(_count);

		while (!_queue.empty())
		{
			const uint32_t _index = _queue.front();
			_queue.pop();

			_order.push_back(_index);

			for (uint32_t _next : _successors[_index])
			{
				if (--_indegree[_next] == 0) _queue.push(_next);
			}
		}

		// 循環していると並べきれずに一部のタスクが結果から抜け落ちる。
		// 「A が書いたものを B が読み、B が書いたものを A が読む」のような
		// 本当に解けない組み合わせなので、システムの読み書き宣言を見直すこと。
		// 抜けた分は実行されなくなるため、登録順の一本鎖へ倒して
		// 「遅いが全部走る」状態にしておく
		if (_order.size() != _count)
		{
			ENGINE_WARNING("[ECS] システムの読み書き依存に循環があります。登録順の直列実行へ切り替えます");
			assert(0 && "システムの依存関係が循環しています");

			_dstGraphVec.resize(_count);
			_dstTaskVec.reserve(_count);

			for (uint32_t _i = 0; _i < _count; ++_i)
			{
				_dstGraphVec[_i].pTask = _taskVec[_i];
				if (_i > 0) _dstGraphVec[_i].dependencyIndices.push_back(_i - 1);
				_dstGraphVec[_i].isTerminal = (_i + 1 == _count);

				_dstTaskVec.push_back(_taskVec[_i]);
			}
			return;
		}

		//--------------------------------------------------------------------------------------
		// 2. 書き込み同士の依存を足しつつ、直接の依存だけを残す
		//
		// 書き込み同士の向きは 1 で決まった並びから取る : 前にいる方が先。
		// 辺が必ず「並びの前 -> 後」を向くので、これ以上並べ替える必要はないし循環もしない。
		//
		// あわせて推移的な辺を落とす。
		// 「A のあとに B、B のあとに C」なら A→C の辺は張らなくてよい。
		// 残すと1つのジョブにぶら下がる後続が一気に増えて、
		// 同じ待ちを何度も数えることになる。
		//
		// 先祖集合はビットで持つ。
		// _ancestors[実行順の位置] = そこへ到達できるタスクの集合
		//--------------------------------------------------------------------------------------
		const size_t _blockCount = (_count + 63) / 64;
		std::vector<uint64_t> _ancestors(_count * _blockCount, 0);

		std::vector<uint32_t> _directPredPosVec = {};	// 使い回し

		_dstGraphVec.resize(_count);

		for (size_t _pos = 0; _pos < _count; ++_pos)
		{
			const SystemTask& _task = *_taskVec[_order[_pos]];

			// 自分より前にいるタスクのうち、ぶつかるものを集める
			_directPredPosVec.clear();
			for (size_t _prevPos = 0; _prevPos < _pos; ++_prevPos)
			{
				const SystemTask& _prevTask = *_taskVec[_order[_prevPos]];

				if (!HasReadWriteDependency(_task, _prevTask) &&
					!HasWriteConflict(_task, _prevTask)) continue;

				_directPredPosVec.push_back(static_cast<uint32_t>(_prevPos));
			}

			// まず「ぶつかる相手の、さらに先祖」だけを集める。
			// 相手は必ず自分より前の位置にあるので、この時点で計算済み
			uint64_t* _pOwn = &_ancestors[_pos * _blockCount];
			for (uint32_t _predPos : _directPredPosVec)
			{
				const uint64_t* _pPred = &_ancestors[_predPos * _blockCount];
				for (size_t _block = 0; _block < _blockCount; ++_block)
				{
					_pOwn[_block] |= _pPred[_block];
				}
			}

			CompiledSystemTask& _compiled = _dstGraphVec[_pos];
			_compiled.pTask = _taskVec[_order[_pos]];

			// 集めた先祖に含まれていない相手 = 他の経路では届かない、本当に必要な辺
			for (uint32_t _predPos : _directPredPosVec)
			{
				if (_pOwn[_predPos / 64] & (1ull << (_predPos % 64))) continue;

				_compiled.dependencyIndices.push_back(_predPos);
			}

			// ぶつかった相手も自分の先祖として記録して、後ろのタスクへ引き継ぐ
			for (uint32_t _predPos : _directPredPosVec)
			{
				_pOwn[_predPos / 64] |= (1ull << (_predPos % 64));
			}
		}

		//--------------------------------------------------------------------------------------
		// 後続の有無を求める
		// 誰からも依存されていないタスクが、そのフェーズの終端になる
		//--------------------------------------------------------------------------------------
		for (const auto& _compiled : _dstGraphVec)
		{
			for (uint32_t _depPos : _compiled.dependencyIndices)
			{
				_dstGraphVec[_depPos].isTerminal = false;
			}
		}

		// 直列実行・デバッグ表示用の並び
		_dstTaskVec.reserve(_count);
		for (const auto& _compiled : _dstGraphVec)
		{
			_dstTaskVec.push_back(_compiled.pTask);
		}
	}

	void SystemManager::RunSerial(const ESystemType& a_type, const SystemContext& a_context)
	{
		// フェーズ検索
		auto _cit = m_compileTaskMap.find(a_type);
		if (_cit == m_compileTaskMap.end()) return;

		// フェーズ内のソートされたシステムを順に回す
		for (auto& _task : _cit->second)
		{
			_task->executeFunc(a_context);
		}
	}

	bool SystemManager::RunParallel(const ESystemType& a_type, const SystemContext& a_context)
	{
		if (!m_isParallelEnabled) return false;
		if (!IsParallelPhase(a_type)) return false;
		if (a_context.pServices == nullptr) return false;

		Thread::JobSystem* _pJobSystem = a_context.pServices->pJobSystem;
		if (_pJobSystem == nullptr || !_pJobSystem->IsRunning()) return false;

		auto _cit = m_compileGraphMap.find(a_type);
		if (_cit == m_compileGraphMap.end()) return false;

		const auto& _graphVec = _cit->second;
		if (_graphVec.empty()) return true;		// 走らせるものがない

		//--------------------------------------------------------------------------------------
		// 1タスク = 1ジョブで積む
		//
		// 依存先は必ず自分より前の位置にあるので、
		// 前から順に作っていけば先行ジョブは必ず出来上がっている
		//--------------------------------------------------------------------------------------
		m_jobBuffer.assign(_graphVec.size(), nullptr);
		m_terminalBuffer.clear();

		for (size_t _i = 0; _i < _graphVec.size(); ++_i)
		{
			const CompiledSystemTask& _compiled = _graphVec[_i];

			m_dependencyBuffer.clear();
			for (uint32_t _depIndex : _compiled.dependencyIndices)
			{
				m_dependencyBuffer.push_back(m_jobBuffer[_depIndex]);
			}

			SystemTask* _pTask = _compiled.pTask;

			// コンテキストは値で持たせる。
			// 参照で捕まえると、呼び出し元のスタックが動いたときに壊れる
			m_jobBuffer[_i] = _pJobSystem->PushJob(
				[_pTask, a_context]() { _pTask->executeFunc(a_context); },
				std::span<Thread::Job* const>(m_dependencyBuffer)
			);

			if (_compiled.isTerminal) m_terminalBuffer.push_back(m_jobBuffer[_i]);
		}

		// 終端をひとつのフェンスへまとめて、そこだけを待つ。
		// WaitForAll() は非同期ロードのジョブまで待ってしまうので使えない
		Thread::Job* _pFence = _pJobSystem->PushJob(
			[]() {},
			std::span<Thread::Job* const>(m_terminalBuffer)
		);

		if (_pFence == nullptr)
		{
			// フェンスが積めなかった = 途中でジョブシステムが止まった。
			// 積んだ分の完了を保証できないので、全体待ちで確実に回収する
			_pJobSystem->WaitForAll();
			return true;
		}

		_pJobSystem->WaitFor(_pFence);
		return true;
	}
}
