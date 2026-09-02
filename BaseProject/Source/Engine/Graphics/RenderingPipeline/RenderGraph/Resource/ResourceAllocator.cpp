#include "ResourceAllocator.h"

#include "VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{
	void ResourceAllocator::CalcHeapSize(const std::vector<VirtualResource>& a_virtualResources)
	{
		// エイリアシングできる仮想リソースのみ配列に入れる
		std::vector<const VirtualResource*> _resources;
		for (const auto& _vRes : a_virtualResources)
		{
			if (!_vRes.IsAliasable()) continue;
			_resources.push_back(&_vRes);
		}

		// ライフタイム開始順に並べ替え
		std::sort(
			_resources.begin(),
			_resources.end(),
			[](const VirtualResource* a_l, const VirtualResource* a_r)
			{
				return a_l->GetFirstPassIndex() < a_r->GetFirstPassIndex();
			}
		);

		// VirtualResource を Alias Slot へ割り当てる
		for (const VirtualResource* _vRes : _resources)
		{
			const uint32_t _firstPass = _vRes->GetFirstPassIndex();
			const uint32_t _lastPass = _vRes->GetLastPassIndex();

			bool _assigned = false;

			// 既存スロットを探す
			for (auto& _slot : m_slots)
			{
				// 前のResrouceのライフタイムが終了しているのなら再利用可能
				if (_slot.lastPassIndex < _firstPass)
				{
					_slot.lastPassIndex = _lastPass;
					_slot.allocationSize = std::max(_slot.allocationSize, _vRes->GetAllocationSize());
					_slot.allocationAlignment = std::max(_slot.allocationAlignment, _vRes->GetAllocationAlignment());

					_assigned = true;
					break;
				}
			}

			// 再利用できる Slot がなければ新たに作成
			if (!_assigned)
			{
				AllocationSlot _newSlot{};
				_newSlot.lastPassIndex = _lastPass;
				_newSlot.allocationSize = _vRes->GetAllocationSize();
				_newSlot.allocationAlignment = _vRes->GetAllocationAlignment();

				m_slots.push_back(_newSlot);
			}
		}

		// 最大同時使用数
		m_maxUageSlot = static_cast<uint32_t>(m_slots.size());

		// ヒープサイズを計算
		m_maxHeapSize = 0;
		for (const auto& _slot : m_slots)
		{
			m_maxHeapSize = Math::Alignment::Up(m_maxHeapSize, _slot.allocationAlignment);
			m_maxHeapSize += _slot.allocationSize;
		}
	}
}