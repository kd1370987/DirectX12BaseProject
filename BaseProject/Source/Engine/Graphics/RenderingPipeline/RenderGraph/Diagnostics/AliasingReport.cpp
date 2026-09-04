#include "AliasingReport.h"

#include "../RenderGraph.h"
#include "../Resource/ResourceAllocator.h"
#include "../Resource/VirtualResource/VirtualResource.h"
#include "../../Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	namespace
	{
		// 席に着けなかった理由。
		// 着けないこと自体は異常ではないが、理由が分かると詰め直しの手がかりになる
		const char* ToUnaliasableReason(const VirtualResource& a_virtual)
		{
			if (a_virtual.IsImported()) return "実体がグラフの外にあるので使い回せません";
			if (a_virtual.IsTemporal()) return "区間がフレームをまたぐので使い回せません";
			if (!a_virtual.HasLifetime()) return "どのパスも触っていません";

			return "割り当ての対象外です";
		}
	}

	AliasingReport BuildAliasingReport(const RenderGraph& a_graph)
	{
		AliasingReport _report = {};

		// 実行順が決まっていなければ、横軸そのものが無い
		const std::vector<CompiledPass>& _compiledVec = a_graph.GetCompiledPasses();
		if (_compiledVec.empty()) return _report;

		//----------------------------------------------------------------------------------
		// 横軸 : 実行順のパス名
		//----------------------------------------------------------------------------------
		_report.passNames.reserve(_compiledVec.size());
		for (const CompiledPass& _compiled : _compiledVec)
		{
			_report.passNames.push_back(_compiled.pPass ? _compiled.pPass->GetName() : std::string("?"));
		}

		//----------------------------------------------------------------------------------
		// 縦軸 : 席
		//----------------------------------------------------------------------------------
		const ResourceAllocator* _pAllocator = a_graph.GetResourceAllocator();
		if (_pAllocator)
		{
			_report.heapSize = _pAllocator->GetMaxHeapSize();

			for (const AllocationSlot& _slot : _pAllocator->GetSlots())
			{
				AliasingReportSlot _reportSlot = {};
				_reportSlot.offset = _slot.offset;
				_reportSlot.reservedSize = _slot.allocationSize;
				_reportSlot.alignment = _slot.allocationAlignment;

				_report.slots.push_back(_reportSlot);
			}
		}

		//----------------------------------------------------------------------------------
		// 矩形 : 仮想リソース1本ずつ
		//----------------------------------------------------------------------------------
		for (const VirtualResource& _virtual : a_graph.GetVirtualResources())
		{
			const AllocationInfo& _info = _virtual.GetAllocationInfo();

			AliasingReportEntry _entry = {};
			_entry.resourceID = _virtual.GetResourceID();
			_entry.name = _virtual.GetName();
			_entry.slotIndex = _info.slotIndex;
			_entry.offset = _info.offset;
			_entry.size = _virtual.GetAllocationSize();

			// 区間を持たないものは 0 のままにしておく(矩形にはならない)
			if (_virtual.HasLifetime())
			{
				_entry.firstPassIndex = _virtual.GetFirstPassIndex();
				_entry.lastPassIndex = _virtual.GetLastPassIndex();
			}

			// 使い回さずに1本ずつ作ったらどうなるか。使い回しの効き目を出すのに使う
			_report.totalResourceSize += _entry.size;

			// 席に着けなかったものは別の入れ物へ。
			// 混ぜるとスロット0の行へ積み上がって、割り当ての様子が読めなくなる
			if (_info.slotIndex == AllocationInfo::INVALID_SLOT_INDEX)
			{
				_entry.pUnaliasableReason = ToUnaliasableReason(_virtual);
				_report.unassignedEntries.push_back(std::move(_entry));
				continue;
			}

			// 直前にこの席を使っていたリソース : エイリアシングバリアの before と同じもの
			if (_info.pPrevVRes)
			{
				_entry.prevResourceID = _info.pPrevVRes->GetResourceID();
				_entry.prevName = _info.pPrevVRes->GetName();
			}

			// 席が実際にどこまで使われたか
			if (_entry.slotIndex < _report.slots.size())
			{
				uint64_t& _usedMax = _report.slots[_entry.slotIndex].usedSizeMax;
				_usedMax = std::max(_usedMax, _entry.size);
			}

			_report.entries.push_back(std::move(_entry));
		}

		// 席ごとに実行順で並べておく。
		// 描く側が並べ替えなくても、繋がりの順に読めるようにする
		std::sort(
			_report.entries.begin(), _report.entries.end(),
			[](const AliasingReportEntry& a_l, const AliasingReportEntry& a_r)
			{
				if (a_l.slotIndex != a_r.slotIndex) return a_l.slotIndex < a_r.slotIndex;
				return a_l.firstPassIndex < a_r.firstPassIndex;
			});

		//----------------------------------------------------------------------------------
		// 実際に積まれたバリア
		//----------------------------------------------------------------------------------
		for (uint32_t _passIdx = 0; _passIdx < static_cast<uint32_t>(_compiledVec.size()); ++_passIdx)
		{
			for (const AliasingBarrier& _barrier : _compiledVec[_passIdx].preAliasingBarriers)
			{
				AliasingReportBarrier _reportBarrier = {};
				_reportBarrier.passIndex = _passIdx;
				_reportBarrier.before = _barrier.before;
				_reportBarrier.after = _barrier.after;

				_report.barriers.push_back(_reportBarrier);
			}
		}

		return _report;
	}
}
