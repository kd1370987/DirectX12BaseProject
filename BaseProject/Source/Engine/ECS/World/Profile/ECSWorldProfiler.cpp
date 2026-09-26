#include "ECSWorldProfiler.h"

#include "../World.h"
#include "../../Archetype/Archetype.h"
#include "../../Archetype/Chunk.h"

namespace Engine::ECS
{
	ECSWorldProfiler::ECSWorldProfiler(const World* a_pOwner)
		: m_pOwner(a_pOwner)
	{}

	ECSWorldProfiler::~ECSWorldProfiler()
	{}

	//======================================================================================
	// 取り直し
	//======================================================================================
	void ECSWorldProfiler::Capture()
	{
		if (!m_pOwner) return;

		m_snapshot.captureCount++;
		m_snapshot.archetypeGeneration = m_pOwner->m_storage.GetArchetypeGeneration();

		CaptureEntity();
		CaptureArchetypes();			// メモリの使用量もここで数える
		CaptureComponentUsage();		// アーキタイプの結果を使う
		CaptureSystemTasks();
		CaptureSchedules();
		CaptureResources();
		CaptureStructuralChange();
	}

	void ECSWorldProfiler::ResetTaskTimings()
	{
		m_taskTimingMap.clear();
	}

	//======================================================================================
	// 計測フック
	//======================================================================================
	void ECSWorldProfiler::RecordTaskTime(const SystemTask* a_pTask, double a_ms)
	{
		TaskTiming& _timing = m_taskTimingMap[a_pTask];

		_timing.lastMs = a_ms;
		_timing.averageMs = (_timing.callCount == 0)
			? a_ms
			: _timing.averageMs + (a_ms - _timing.averageMs) * TASK_AVERAGE_RATE;
		_timing.maxMs = std::max(_timing.maxMs, a_ms);
		_timing.callCount++;
	}

	//======================================================================================
	// Capture の中身
	//======================================================================================
	void ECSWorldProfiler::CaptureEntity()
	{
		const EntityManager& _entityManager = m_pOwner->m_storage.GetEntityManager();

		ECSEntityProfile& _out = m_snapshot.entity;
		_out.aliveCount = _entityManager.GetAliveCount();
		_out.slotCount = _entityManager.GetSlotCount();
		_out.recycleCount = _entityManager.GetRecycleCount();
	}

	void ECSWorldProfiler::CaptureArchetypes()
	{
		const ArchetypeManager& _archetypeManager = m_pOwner->m_storage.GetArchetypeManager();
		const std::vector<ComponentMeta>& _metaVec = m_pOwner->GetAllComponentMetaData();

		//------------------------------------------------------------------
		// チャンクのメモリ
		//------------------------------------------------------------------
		const ChunkAllocator& _allocator = _archetypeManager.GetChunkAllocator();

		ECSChunkMemoryProfile& _memory = m_snapshot.memory;
		_memory = {};
		_memory.blockCount = _allocator.GetBlockCount();
		_memory.blockChunkNum = _allocator.GetBlockChunkNum();
		_memory.totalChunkCount = _allocator.GetTotalChunkCount();
		_memory.freeChunkCount = _allocator.GetFreeChunkCount();
		_memory.usedChunkCount = _memory.totalChunkCount - _memory.freeChunkCount;
		_memory.chunkBytes = ChunkAllocator::CHUNK_MEMORY_SIZE;
		_memory.reservedBytes = _memory.totalChunkCount * ChunkAllocator::CHUNK_MEMORY_SIZE;

		//------------------------------------------------------------------
		// アーキタイプ
		//------------------------------------------------------------------
		const auto& _archetypeVec = _archetypeManager.GetArchetypeVec();

		auto& _outVec = m_snapshot.archetypes;
		_outVec.clear();
		_outVec.reserve(_archetypeVec.size());

		for (size_t _i = 0; _i < _archetypeVec.size(); ++_i)
		{
			const Archetype& _archetype = *_archetypeVec[_i];

			ECSArchetypeProfile& _out = _outVec.emplace_back();
			_out.index = _i;
			_out.signature = _archetype.signature;
			_out.chunkCapacity = _archetype.chunkCapacity;
			_out.maxAlign = _archetype.maxAlign;

			// コンポーネント配列の配置
			_out.entityStride = sizeof(Entity);
			_out.layoutBytes = sizeof(Entity) * _archetype.chunkCapacity;	// 先頭のエンティティ配列
			for (const auto& [_typeID, _layout] : _archetype.layoutMap)
			{
				ECSComponentLayoutProfile& _comp = _out.components.emplace_back();
				_comp.typeID = _typeID;
				_comp.offset = _layout.offset;
				_comp.stride = _layout.stride;
				if (_typeID < _metaVec.size())
				{
					const ComponentMeta& _meta = _metaVec[_typeID];
					_comp.name = _meta.name;
					_comp.size = _meta.compSize;
					_comp.align = _meta.compAlign;
				}

				_out.entityStride += _layout.stride;
				_out.layoutBytes = std::max(_out.layoutBytes, _layout.offset + _layout.stride * _archetype.chunkCapacity);
			}

			// layoutMap は順不同なので、チャンク内の並び順にそろえる
			std::sort(_out.components.begin(), _out.components.end(),
				[](const ECSComponentLayoutProfile& a_l, const ECSComponentLayoutProfile& a_r) { return a_l.offset < a_r.offset; });

			// チャンク
			_out.chunks.reserve(_archetype.chunks.size());
			for (const Chunk* _pChunk : _archetype.chunks)
			{
				ECSChunkProfile& _chunk = _out.chunks.emplace_back();
				_chunk.address = reinterpret_cast<uintptr_t>(_pChunk->data);
				_chunk.count = _pChunk->count;
				_chunk.isFreeChunk = (_pChunk == _archetype.pFreeChunk);

				_out.entityCount += _pChunk->count;
			}

			_memory.liveBytes += static_cast<size_t>(_out.entityCount) * _out.entityStride;
		}
	}

	void ECSWorldProfiler::CaptureComponentUsage()
	{
		const std::vector<ComponentMeta>& _metaVec = m_pOwner->GetAllComponentMetaData();

		auto& _outVec = m_snapshot.components;
		_outVec.clear();
		_outVec.resize(_metaVec.size());

		for (size_t _i = 0; _i < _metaVec.size(); ++_i)
		{
			ECSComponentUsageProfile& _out = _outVec[_i];
			_out.typeID = static_cast<ComponentTypeID>(_i);
			_out.name = _metaVec[_i].name;
			_out.size = _metaVec[_i].compSize;
			_out.align = _metaVec[_i].compAlign;
		}

		// 直前に取ったアーキタイプの結果から数える
		for (const ECSArchetypeProfile& _archetype : m_snapshot.archetypes)
		{
			for (const ECSComponentLayoutProfile& _comp : _archetype.components)
			{
				if (_comp.typeID >= _outVec.size()) continue;

				_outVec[_comp.typeID].archetypeCount++;
				_outVec[_comp.typeID].entityCount += _archetype.entityCount;
			}
		}
	}

	void ECSWorldProfiler::CaptureSystemTasks()
	{
		const SystemManager& _systemManager = m_pOwner->m_systemManager;
		const auto& _taskMap = _systemManager.GetCompileTaskMap();
		const auto& _compiledMap = _systemManager.GetCompiledTaskMap();
		const auto& _reportMap = _systemManager.GetScheduleReportMap();
		const uint64_t _generation = m_snapshot.archetypeGeneration;

		auto& _outVec = m_snapshot.systemTasks;
		_outVec.clear();

		// フェーズの定義順に並べる
		for (int _phase = 0; _phase < static_cast<int>(ESystemType::Num); ++_phase)
		{
			const ESystemType _type = static_cast<ESystemType>(_phase);

			auto _it = _taskMap.find(_type);
			if (_it == _taskMap.end()) continue;

			// 待ち合わせと診断(並びはソート結果と同じ)
			auto _compiledIt = _compiledMap.find(_type);
			const std::vector<CompileTask>* _pCompiledVec =
				(_compiledIt != _compiledMap.end()) ? &_compiledIt->second : nullptr;

			auto _reportIt = _reportMap.find(_type);
			const PhaseScheduleReport* _pReport =
				(_reportIt != _reportMap.end()) ? &_reportIt->second : nullptr;

			uint32_t _order = 0;
			for (size_t _index = 0; _index < _it->second.size(); ++_index)
			{
				const SystemTask* _pTask = _it->second[_index];
				if (!_pTask) continue;

				ECSSystemTaskProfile& _out = _outVec.emplace_back();
				_out.phase = _type;
				_out.order = _order++;
				_out.name = _pTask->name;
				_out.readNames = ToComponentNames(_pTask->readSig);
				_out.writeNames = ToComponentNames(_pTask->writeSig);

				// 実行のされ方
				_out.isJob = (_pTask->exec == ETaskExec::Job);
				if (_pCompiledVec && _index < _pCompiledVec->size())
				{
					for (uint32_t _waitIndex : (*_pCompiledVec)[_index].waitIndices)
					{
						if (_waitIndex >= _pCompiledVec->size()) continue;
						const SystemTask* _pWait = (*_pCompiledVec)[_waitIndex].pTask;
						if (_pWait) _out.waitNames.push_back(_pWait->name);
					}
				}
				if (_pReport)
				{
					const auto& _cyclicVec = _pReport->cyclicTaskVec;
					_out.isCyclic = std::find(_cyclicVec.begin(), _cyclicVec.end(), _pTask) != _cyclicVec.end();

					for (const ScheduleAmbiguity& _amb : _pReport->ambiguityVec)
					{
						if (_amb.pEarlier == _pTask || _amb.pLater == _pTask) _out.ambiguityCount++;
					}
				}

				// クエリ : 一度も回っていないもの(カスタムタスクを含む)は持っていない
				const QueryCache& _query = _pTask->query;
				_out.hasQuery = (_query.generation != QueryCache::INVALID_GENERATION);
				if (_out.hasQuery)
				{
					_out.isQueryStale = _query.IsStale(_generation);
					_out.matchedChunkCount = _query.chunkVec.size();

					// 古いキャッシュのチャンクは別のアーキタイプへ貸し直されているかもしれないので数えない
					if (!_out.isQueryStale)
					{
						for (const Chunk* _pChunk : _query.chunkVec)
						{
							if (_pChunk) _out.matchedEntityCount += _pChunk->count;
						}
					}
				}

				// 実行時間
				auto _timingIt = m_taskTimingMap.find(_pTask);
				if (_timingIt != m_taskTimingMap.end())
				{
					const TaskTiming& _timing = _timingIt->second;
					_out.lastMs = _timing.lastMs;
					_out.averageMs = _timing.averageMs;
					_out.maxMs = _timing.maxMs;
					_out.callCount = _timing.callCount;
				}
			}
		}
	}

	void ECSWorldProfiler::CaptureSchedules()
	{
		const auto& _reportMap = m_pOwner->m_systemManager.GetScheduleReportMap();

		auto& _outVec = m_snapshot.schedules;
		_outVec.clear();

		// フェーズの定義順に並べる
		for (int _phase = 0; _phase < static_cast<int>(ESystemType::Num); ++_phase)
		{
			const ESystemType _type = static_cast<ESystemType>(_phase);

			auto _it = _reportMap.find(_type);
			if (_it == _reportMap.end()) continue;

			const PhaseScheduleReport& _report = _it->second;

			ECSPhaseScheduleProfile& _out = _outVec.emplace_back();
			_out.phase = _type;
			_out.isSorted = _report.isSorted;

			for (const SystemTask* _pTask : _report.cyclicTaskVec)
			{
				if (_pTask) _out.cyclicTaskNames.push_back(_pTask->name);
			}

			for (const ScheduleAmbiguity& _amb : _report.ambiguityVec)
			{
				if (!_amb.pEarlier || !_amb.pLater) continue;

				ECSScheduleAmbiguityProfile& _outAmb = _out.ambiguities.emplace_back();
				_outAmb.earlierName = _amb.pEarlier->name;
				_outAmb.laterName = _amb.pLater->name;
				_outAmb.conflictNames = ToComponentNames(_amb.conflictSig);
			}
		}
	}

	void ECSWorldProfiler::CaptureResources()
	{
		auto& _outVec = m_snapshot.resources;
		_outVec.clear();

		for (const auto& [_id, _upWrapper] : m_pOwner->m_resourceStore.GetResourceMap())
		{
			if (!_upWrapper) continue;

			ECSResourceProfile& _out = _outVec.emplace_back();
			_out.id = _id;
			_out.name = std::string(_upWrapper->GetTypeName());
			_out.size = _upWrapper->GetTypeSize();
		}

		// unordered_map の並びは毎回変わりうるので、IDでそろえる
		std::sort(_outVec.begin(), _outVec.end(),
			[](const ECSResourceProfile& a_l, const ECSResourceProfile& a_r) { return a_l.id < a_r.id; });
	}

	void ECSWorldProfiler::CaptureStructuralChange()
	{
		const CommandBuffer& _commandBuffer = m_pOwner->m_commandBuffer;

		ECSStructuralChangeProfile& _out = m_snapshot.structural;
		_out.created = m_createdSinceCapture;
		_out.changed = m_changedSinceCapture;
		_out.removed = m_removedSinceCapture;

		_out.pendingCreate = _commandBuffer.GetCreateCount();
		_out.pendingChange = _commandBuffer.GetChangeCount();
		_out.pendingRemove = _commandBuffer.GetRemoveCount();
		_out.pendingRefresh = _commandBuffer.GetRefreshCount();

		// 次の Capture までの分を数え直す
		m_createdSinceCapture = 0;
		m_changedSinceCapture = 0;
		m_removedSinceCapture = 0;
	}

	std::vector<std::string> ECSWorldProfiler::ToComponentNames(const Signature& a_sig) const
	{
		const std::vector<ComponentMeta>& _metaVec = m_pOwner->GetAllComponentMetaData();

		std::vector<std::string> _names = {};
		for (size_t _i = 0; _i < a_sig.size() && _i < _metaVec.size(); ++_i)
		{
			if (a_sig.test(_i)) _names.push_back(_metaVec[_i].name);
		}
		return _names;
	}
}
