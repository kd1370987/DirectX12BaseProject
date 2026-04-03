#pragma once

namespace Engine::Storage
{
	// ハンドルを管理するクラス
	template<typename T>
	class HandleStorage
	{
	public:

		// 生成
		void Create(UINT a_maxCount = 0);

		// データの追加
		Engine::Resource::Handle<T> Allocate();

		// データの削除
		void Remove(const Engine::Resource::Handle<T>& a_handle);

		// 有効かどうか : true = 使える値、有効
		bool IsValid(const Engine::Resource::Handle<T>& a_handle) const;

	private:
		std::vector<Engine::Resource::Generation> m_genVec = {};		// 世代配列
		std::queue<Engine::Resource::Index> m_indexQueue = {};			// 使用ハンドル行列

		UINT m_maxCount = 0;		// ハンドルの最大数
		UINT m_currentCount = 0;	// 現在のハンドル数
	};

	template<typename T>
	inline void HandleStorage<T>::Create(UINT a_maxCount)
	{
		m_indexQueue = {};
		for (UINT _idx = 0; _idx < a_maxCount; ++_idx)
		{
			m_indexQueue.push(_idx);
		}

		m_maxCount = a_maxCount;

		std::vector<Engine::Resource::Generation> _genVec = { 0 };
	}

	template<typename T>
	inline Engine::Resource::Handle<T> HandleStorage<T>::Allocate()
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
				assert(0 && "ストレージの設定上限に達しました");
				return Engine::Resource::Handle<T>();
			}
		}

		// インデックス取得
		Engine::Resource::Index _idx = m_indexQueue.front();
		m_indexQueue.pop();

		// ハンドル作成
		Engine::Resource::Handle<T> _handle = {};
		_handle.idx = _idx;
		_handle.gen = m_genVec[_idx];
		return _handle;
	}
	template<typename T>
	inline void HandleStorage<T>::Remove(const Engine::Resource::Handle<T>& a_handle)
	{
		// 世代が一致しているかどうか
		if (IsValid(a_handle))
		{
			// 世代を進める
			m_genVec[a_handle.idx]++;

			// インデックスを未使用キューに戻す
			m_indexQueue.push(a_handle.idx);
		}
	}
	template<typename T>
	inline bool HandleStorage<T>::IsValid(const Engine::Resource::Handle<T>& a_handle) const
	{
		// 世代が一致しているかどうか
		if(m_genVec[a_handle.idx] != a_handle.gen)
		{
			// 世代が一致していない = 無効
			return false;
		}

		// 世代が一致している = 有効
		return true;
	}
}