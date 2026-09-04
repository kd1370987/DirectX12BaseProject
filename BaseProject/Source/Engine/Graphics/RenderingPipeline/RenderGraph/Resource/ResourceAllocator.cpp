#include "ResourceAllocator.h"

#include "VirtualResource/VirtualResource.h"

namespace Engine::Graphics::Pipeline
{

	void ResourceAllocator::CalcAllocation(std::vector<VirtualResource>& a_virtualResources)
	{
		m_slots.clear();

		// エイリアシングできる仮想リソースを集める
		std::vector<VirtualResource*> _pResources = {};
		for (auto& _vRes : a_virtualResources)
		{
			if (!_vRes.IsAliasable()) continue;
			_pResources.push_back(&_vRes);
		}

		// ライフタイム順に並べ替える
		std::sort(
			_pResources.begin(),
			_pResources.end(),
			[](const VirtualResource* a_l,const VirtualResource* a_r)
			{
				return a_l->GetFirstPassIndex() < a_r->GetFirstPassIndex();
			}
		);

		// VirtaulResource を Slot へ振り分ける
		for (VirtualResource* _vRes : _pResources)
		{
			const uint32_t _firstPass = _vRes->GetFirstPassIndex();
			const uint32_t _lastPass = _vRes->GetLastPassIndex();

			bool _assigned = false;

			// 既存スロットを探す
			for(size_t _i = 0; _i < m_slots.size(); ++_i)
			{
				auto& _slot = m_slots[_i];

				// 前のResrouceのライフタイムが終了しているのなら再利用可能
				if (_slot.lastPassIndex < _firstPass)
				{
					
					// 仮想リソースに一時的にスロットインデックスを記録
					AllocationInfo _info = _vRes->GetAllocationInfo();
					_info.slotIndex = static_cast<uint32_t>(_i);
					_info.pPrevVRes = _slot.pLastResource;
					_vRes->SetAllocationInfo(_info);

					// スロットの情報を更新
					_slot.lastPassIndex = _lastPass;
					_slot.pLastResource = _vRes;
					_slot.allocationSize = std::max(_slot.allocationSize, _vRes->GetAllocationSize());
					_slot.allocationAlignment = std::max(_slot.allocationAlignment, _vRes->GetAllocationAlignment());

					_assigned = true;

					break;
				}
			}

			// 再利用できる Slot がなければ新たに作成
			if (!_assigned)
			{
				// スロットの新規作成
				AllocationSlot _newSlot{};
				_newSlot.lastPassIndex = _lastPass;
				_newSlot.allocationSize = _vRes->GetAllocationSize();
				_newSlot.allocationAlignment = _vRes->GetAllocationAlignment();
				_newSlot.pLastResource = _vRes;

				// 仮想リソースに一時的にスロットインデックスを記録
				AllocationInfo _info = _vRes->GetAllocationInfo();
				_info.slotIndex = static_cast<uint32_t>(m_slots.size());
				_info.pPrevVRes = nullptr;
				_vRes->SetAllocationInfo(_info);

				m_slots.push_back(_newSlot);
			}
		}

		m_maxUageSlot = static_cast<uint32_t>(m_slots.size());

		// 各スロットのメモリ上のオフセットと、全体のヒープサイズを計算
		m_maxHeapSize = 0;
		for (auto& _slot : m_slots)
		{
			// アライメントを考慮してスロットの開始オフセットを決定
			m_maxHeapSize = Math::Alignment::Up(m_maxHeapSize,_slot.allocationAlignment);
			_slot.offset = m_maxHeapSize;

			// 次のスロット開始位置のためにサイズを加算
			m_maxHeapSize += _slot.allocationSize;
		}

		// 確定したオフセットを仮想リソースに書き込む
		for (VirtualResource* _vRes : _pResources)
		{
			AllocationInfo _info = _vRes->GetAllocationInfo();
			const auto& _slot = m_slots[_info.slotIndex];

			// スロットの開始オフセットをベースに、リソース自身のアライメントを最終確認して書き込み。
			// 大きさと詰め方はリソース自身が持っているので、ここへ写さない
			_info.offset = Math::Alignment::Up(_slot.offset,_vRes->GetAllocationAlignment());

			_vRes->SetAllocationInfo(_info);
		}
	}
}