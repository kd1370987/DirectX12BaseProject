#include "IndexRangeAllocator.h"
namespace Engine::Graphics
{
	void IndexRangeAllocator::Init(uint32_t a_maxCount)
	{
		m_maxCount = a_maxCount;
		m_freeBlocks.clear();
		while (!m_pendingFrees.empty()) m_pendingFrees.pop();

		// 最初は全領域が1つのフリーブロック
		m_freeBlocks.push_back({ 0, m_maxCount });
	}
	IndexRangeHandle IndexRangeAllocator::AllocateRange(uint32_t a_count)
	{
		for (auto _it = m_freeBlocks.begin(); _it != m_freeBlocks.end(); ++_it)
		{
			if (_it->count >= a_count)
			{
				uint32_t _startIdx = _it->startIndex;

				if (_it->count == a_count)
				{
					m_freeBlocks.erase(_it);
				}
				else
				{
					_it->startIndex += a_count;
					_it->count -= a_count;
				}

				// アロケーションIDを発行する
				return { _startIdx, a_count, m_currentAllocationId++ };
			}
		}
		ENGINE_ERRLOG(false,"メッシュバッファのメモリ領域がいっぱいです");
		return { 0, 0, 0 }; // 確保失敗
	}
	void IndexRangeAllocator::FreeRange(const IndexRangeHandle& a_handle, uint64_t a_currentFenceValue)
	{
		if (!a_handle.isValid()) return;

		// 次のフレーム（または指定フェンス完了後）に解放されるようにキューに積む
		m_pendingFrees.push({
			{ a_handle.startIndex, a_handle.count },
			a_currentFenceValue
		});
	}
	void IndexRangeAllocator::UpdateFrees(uint64_t a_completedFenceValue)
	{
		bool _needsMerge = false;

		while (!m_pendingFrees.empty())
		{
			if (m_pendingFrees.front().releaseFenceValue <= a_completedFenceValue)
			{
				// GPUが使い終わったので、フリーリストに返却
				m_freeBlocks.push_back(m_pendingFrees.front().block);
				m_pendingFrees.pop();
				_needsMerge = true;
			}
			else
			{
				// キューは順番通りなので、先頭がまだなら以降もまだ
				break;
			}
		}

		if (_needsMerge)
		{
			Merge();
		}
	}
	void IndexRangeAllocator::Merge()
	{
		if (m_freeBlocks.size() <= 1) return;

		std::sort(m_freeBlocks.begin(), m_freeBlocks.end(),
			[](const FreeBlock& a_a, const FreeBlock& a_b) {
				return a_a.startIndex < a_b.startIndex;
			}
		);

		size_t _writeIdx = 0;
		for (size_t _readIdx = 1; _readIdx < m_freeBlocks.size(); ++_readIdx)
		{
			if (m_freeBlocks[_writeIdx].startIndex + m_freeBlocks[_writeIdx].count == m_freeBlocks[_readIdx].startIndex)
			{
				m_freeBlocks[_writeIdx].count += m_freeBlocks[_readIdx].count;
			}
			else
			{
				_writeIdx++;
				m_freeBlocks[_writeIdx] = m_freeBlocks[_readIdx];
			}
		}
		m_freeBlocks.resize(_writeIdx + 1);
	}
}



