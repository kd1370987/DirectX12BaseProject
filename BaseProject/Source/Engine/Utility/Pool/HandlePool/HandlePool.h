#pragma once

namespace Engine::Storage
{
	// ハンドルを管理するクラス
	template<typename T>
	class HandlePool
	{
	public:
		/// <summary>
		/// ハンドルの初期化 : 最大サイズを指定した場合のみ上限チェックを設ける
		/// </summary>
		/// <param name="a_maxCount">最大サイズ</param>
		void Create(uint32_t a_maxCount = 0);

		/// <summary>
		/// ハンドルの割り当て
		/// </summary>
		/// <returns>割り当て済みのハンドル</returns>
		Handle<T> Allocate();

		/// <summary>
		/// ハンドルの削除
		/// </summary>
		/// <param name="a_handle">削除したいハンドル</param>
		void Remove(const Handle<T>& a_handle);

		/// <summary>
		/// データの有効チェック
		/// </summary>
		/// <param name="a_handle">確認したいハンドル</param>
		bool IsValid(const Handle<T>& a_handle) const;

	private:
		std::vector<uint16_t> m_genVec = {};		// 世代配列
		std::queue<uint16_t> m_indexQueue = {};			// 使用ハンドル行列

		uint32_t m_maxCount = 0;		// ハンドルの最大数
		uint32_t m_currentCount = 0;	// 現在のハンドル数
	};

	template<typename T>
	inline void HandlePool<T>::Create(uint32_t a_maxCount)
	{
		m_indexQueue = {};
		for (uint32_t _idx = 0; _idx < a_maxCount; ++_idx)
		{
			m_indexQueue.push(_idx);
		}
		m_maxCount = a_maxCount;

		// ０で初期化
		m_genVec.assign(a_maxCount, 0);
	}

	template<typename T>
	inline Handle<T> HandlePool<T>::Allocate()
	{
		if (m_indexQueue.empty())
		{
			if (m_maxCount == 0)
			{
				// インデックス上限が設定されていなければ、インデックスを追加
				m_indexQueue.push(m_currentCount);
				m_currentCount++;
				m_genVec.push_back(0);
			}
			else
			{
				// インデックス上限が設定されていれば、上限に達したことをアサート
				ENGINE_ERRLOG(false, "ストレージの設定上限に達しました");
				return Handle<T>();
			}
		}

		// インデックス取得
		uint16_t _idx = m_indexQueue.front();
		m_indexQueue.pop();

		// ハンドル作成
		Handle<T> _handle(_idx, m_genVec[_idx]);
		return _handle;
	}
	template<typename T>
	inline void HandlePool<T>::Remove(const Handle<T>& a_handle)
	{
		if (!a_handle.IsValid()) return;

		// 世代が一致しているかどうか
		if (IsValid(a_handle))
		{
			// 世代を進める
			m_genVec[a_handle.GetIndex()]++;

			// インデックスを未使用キューに戻す
			m_indexQueue.push(a_handle.GetIndex());
		}
	}
	template<typename T>
	inline bool HandlePool<T>::IsValid(const Handle<T>& a_handle) const
	{
		// 世代が一致しているかどうか
		if (m_genVec.size() <= a_handle.GetIndex())
		{
			return false;
		}
		if (m_genVec[a_handle.GetIndex()] != a_handle.GetGeneration())
		{
			// 世代が一致していない = 無効
			return false;
		}

		// 世代が一致している = 有効
		return true;
	}
}