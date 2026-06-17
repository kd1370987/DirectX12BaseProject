#pragma once
namespace Engine::Pool
{
	template<typename T>
	class RangePool
	{
	public:

		RangePool() = default;
		~RangePool() = default;

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="a_maxCount">確保領域</param>
		void Init(size_t a_maxCount);

		/// <summary>
		/// 指定した数の連続領域を確保
		/// </summary>
		/// <param name="a_count">確保サイズ</param>
		/// <returns>型ハンドル</returns>
		RangeHandle<T> AllocateRange(uint32_t a_count);

		/// <summary>
		/// 領域の解放
		/// </summary>
		/// <param name="a_handle">削除データハンドル</param>
		void FreeRange(const RangeHandle<T>& a_handle);

		/// <summary>
		/// 参照
		/// </summary>
		/// <param name="a_handle">ハンドル</param>
		/// <returns>std::spanで変える</returns>
		std::span<T> RefRange(const RangeHandle<T>& a_handle);

		/// <summary>
		/// 読み取り参照
		/// </summary>
		/// <param name="a_handle">ハンドル</param>
		/// <returns>std::spanで変える</returns>
		std::span<const T> GetRange(const RangeHandle<T>& a_handle) const;

		/// <summary>
		/// プール内に存在するかチェック
		/// </summary>
		/// <param name="a_handle">ハンドル</param>
		/// <returns>あれば true </returns>
		bool IsValid(const RangeHandle<T>& a_handle) const;

		/// <summary>
		/// 全データを参照 : 読み取り専用
		/// </summary>
		/// <returns></returns>
		const std::vector<T>& GetAllData() const;

	private:

		/// <summary>
		/// 結合できる領域を探して結合
		/// </summary>
		void Merge();

	private:

		// 確保領域
		struct FreeBlock
		{
			uint32_t startIndex;
			uint32_t count;
		};

		// データ
		std::vector<T> m_data;
		std::vector<uint32_t> m_generations;
		std::vector<FreeBlock> m_freeBlocks;
		uint32_t m_currentGeneration = 0;
	};

	template<typename T>
	inline void RangePool<T>::Init(size_t a_maxCount)
	{
		m_data.resize(a_maxCount);
		m_generations.resize(a_maxCount,0);

		// 全領域が一つのフリーブロック
		m_freeBlocks.clear();
		m_freeBlocks.push_back({ 0,static_cast<uint32_t>(a_maxCount) });
	}
	template<typename T>
	inline RangeHandle<T> RangePool<T>::AllocateRange(uint32_t a_count)
	{
		for (auto _it = m_freeBlocks.begin(); _it != m_freeBlocks.end(); ++_it)
		{
			if (_it->count >= a_count)
			{
				// 開始位置を記録
				uint32_t _startIdx = _it->startIndex;

				// ブロックを分割
				if (_it->count == a_count)
				{
					m_freeBlocks.erase(_it);
				}
				else
				{
					_it->startIndex += a_count;
					_it->count -= a_count;
				}

				// 領域の世代を更新
				uint32_t _currentGen = ++m_currentGeneration;
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					m_generations[_startIdx + _i] = _currentGen;
				}
				return { _startIdx,a_count,_currentGen };
			}
		}

		// 領域確保失敗
		return {};
	}
	template<typename T>
	inline void RangePool<T>::FreeRange(const RangeHandle<T>& a_handle)
	{
		// 領域内にあるかチェック
		if (!IsValid(a_handle)) return;

		// 世代をクリアして無効化
		for (uint32_t _i = 0; _i < a_handle.count; ++_i)
		{
			m_generations[a_handle.startIndex + _i]++;
		}

		// フリーリストに返却
		m_freeBlocks.push_back({a_handle.startIndex,a_handle.count});

		// 領域の結合
		Merge();
	}
	template<typename T>
	inline std::span<T> RangePool<T>::RefRange(const RangeHandle<T>& a_handle)
	{
		// ハンドル内にあれば返す
		if (IsValid(a_handle))
		{
			return std::span<T>(&m_data[a_handle.startIndex],a_handle.count);
		}
		// なければ空の配列を返す
		return {};
	}
	template<typename T>
	inline std::span<const T> RangePool<T>::GetRange(const RangeHandle<T>& a_handle) const
	{
		// ハンドル内にあれば返す
		if (IsValid(a_handle))
		{
			return std::span<const T>(&m_data[a_handle.startIndex], a_handle.count);
		}
		// なければ空の配列を返す
		return {};
	}
	template<typename T>
	inline bool RangePool<T>::IsValid(const RangeHandle<T>& a_handle) const
	{
		if (a_handle.startIndex + a_handle.count > m_data.size()) 
		{
			return false;
		}
		// 先頭の世代が一致しているか確認
		return m_generations[a_handle.startIndex] == a_handle.generation;
	}
	template<typename T>
	inline const std::vector<T>& RangePool<T>::GetAllData() const
	{
		return m_data;
	}
	template<typename T>
	inline void RangePool<T>::Merge()
	{
		// 【追加】空、またはブロックが1つしかないなら結合の必要なし
		if (m_freeBlocks.size() <= 1) return;

		// 開始位置が小さい順に並べていく
		std::sort(m_freeBlocks.begin(),m_freeBlocks.end(),
			[](const FreeBlock& a_a, const FreeBlock& a_b)
			{
				return a_a.startIndex < a_b.startIndex;
			}
		);

		// 前詰めで配列を再構築
		size_t _writeIdx = 0;
		for (size_t _readIdx = 1; _readIdx<m_freeBlocks.size();++_readIdx)
		{
			// 現在の領域の終わりが、次の領域の開始位置と同じなら結合
			if (m_freeBlocks[_writeIdx].startIndex + m_freeBlocks[_writeIdx].count == m_freeBlocks[_readIdx].startIndex)
			{
				m_freeBlocks[_writeIdx].count += m_freeBlocks[_readIdx].count;
			}
			else
			{
				// 結合できなければ次の書き込み位置へ進めてコピー
				_writeIdx++;
				m_freeBlocks[_writeIdx] = m_freeBlocks[_readIdx];
			}
		}

		// 結合して余った後ろの要素を切り捨てる
		m_freeBlocks.resize(_writeIdx + 1);
	}
}