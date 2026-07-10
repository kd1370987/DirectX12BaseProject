#pragma once
namespace Engine
{
	/// <summary>
	/// バッファと１対１で持つアロケータークラス
	/// </summary>
	template<typename T>
	class RangeAllocator
	{
	public:
		RangeAllocator() = default;
		~RangeAllocator() = default;

		/// <summary>
		/// 初期化 : 全体で一つのブロックを作る
		/// </summary>
		/// <param name="a_maxCount">最大領域数</param>
		void Init(uint32_t a_maxCount);

		/// <summary>
		/// 指定されたサイズの領域を確保する
		/// </summary>
		/// <param name="a_count">サイズ</param>
		/// <returns>割り当てられた領域</returns>
		RangeHandle<T> AllocateRange(uint32_t a_count);

		/// <summary>
		/// 指定した領域の解放 : 現在のフレームでは解放しずにフェンス値によって遅延開放される
		/// </summary>
		/// <param name="a_handle">指定した領域</param>
		/// <param name="a_currentFenceValue">現在のフェンス値</param>
		void FreeRange(const RangeHandle<T>& a_handle, uint64_t a_currentFenceValue);

		// 毎フレームの頭で呼ぶ：GPUが使い終わった領域をフリーリストに戻す
		
		/// <summary>
		/// 毎フレーム最初に呼ぶ : GPUが使い終わった領域をフリーリストに戻す
		/// メッシュバッファアロケータ側のバッファと同期する必要あり
		/// </summary>
		/// <param name="a_completedFenceValue">現在のフェンス値</param>
		void UpdateFrees(uint64_t a_completedFenceValue);


		uint32_t GetMaxCount() const { return m_maxCount; }
	private:

		/// <summary>
		/// 空いている領域を集めて塊にする
		/// </summary>
		void Merge();

	private:
		struct FreeBlock
		{
			uint32_t startIndex;
			uint32_t count;
		};

		// 遅延解放用の構造体
		struct PendingFree
		{
			FreeBlock block;
			uint64_t releaseFenceValue; // このフェンス値を越えたら再利用可能
		};

		std::vector<FreeBlock> m_freeBlocks;
		std::queue<PendingFree> m_pendingFrees;
		uint32_t m_currentAllocationId = 1;
		uint32_t m_maxCount = 0;
	};
	template<typename T>
	inline void RangeAllocator<T>::Init(uint32_t a_maxCount)
	{
		m_maxCount = a_maxCount;
		m_freeBlocks.clear();
		while (!m_pendingFrees.empty()) m_pendingFrees.pop();

		m_freeBlocks.push_back({ 0, m_maxCount });
	}
	template<typename T>
	inline RangeHandle<T> RangeAllocator<T>::AllocateRange(uint32_t a_count)
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
				// 型安全なハンドルを返す
				return { _startIdx, a_count, m_currentAllocationId++ };
			}
		}
		ENGINE_ERRLOG(false, "メガバッファのメモリ領域がいっぱいです");
		return { std::numeric_limits<uint32_t>::max(), 0, 0 }; // 無効なハンドル
	}
	template<typename T>
	inline void RangeAllocator<T>::FreeRange(const RangeHandle<T>& a_handle, uint64_t a_currentFenceValue)
	{
		if (!a_handle.IsValid()) return;

		m_pendingFrees.push({
			{ a_handle.startIndex, a_handle.count },
			a_currentFenceValue
			});
	}
	template<typename T>
	inline void RangeAllocator<T>::UpdateFrees(uint64_t a_completedFenceValue)
	{
		bool _needsMerge = false;

		while (!m_pendingFrees.empty())
		{
			if (m_pendingFrees.front().releaseFenceValue <= a_completedFenceValue)
			{
				ENGINE_LOG(
					"FreeListの解放 : start %d, count %d",
					m_pendingFrees.front().block.startIndex, m_pendingFrees.front().block.count
				);
				m_freeBlocks.push_back(m_pendingFrees.front().block);
				m_pendingFrees.pop();
				_needsMerge = true;
			}
			else
			{
				break;
			}
		}
		if (_needsMerge) Merge();
	}
	template<typename T>
	inline void RangeAllocator<T>::Merge()
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