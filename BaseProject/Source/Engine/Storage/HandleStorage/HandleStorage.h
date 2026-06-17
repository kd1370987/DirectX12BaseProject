#pragma once

namespace Engine::Storage
{
	// ハンドルを管理するクラス
	template<typename T>
	class HandleStorage
	{
	public:

		// 生成
		// 上限値を設定できる。設定しない場合は上限なく増えていく
		void Create(UINT a_maxCount = 0);

		// データの追加
		Handle<T> Allocate();

		// データの削除
		void Remove(const Handle<T>& a_handle);

		// 有効かどうか : true = 使える値、有効
		bool IsValid(const Handle<T>& a_handle) const;

	private:
		std::vector<uint16_t> m_genVec = {};		// 世代配列
		std::queue<uint16_t> m_indexQueue = {};			// 使用ハンドル行列

		uint32_t m_maxCount = 0;		// ハンドルの最大数
		uint32_t m_currentCount = 0;	// 現在のハンドル数
	};

	template<typename T>
	inline void HandleStorage<T>::Create(uint32_t a_maxCount)
	{
		m_indexQueue = {};
		for (uint32_t _idx = 0; _idx < a_maxCount; ++_idx)
		{
			m_indexQueue.push(_idx);
		}
		m_maxCount = a_maxCount;

		// ０で初期化
		m_genVec.assign(a_maxCount,0);
	}

	template<typename T>
	inline Handle<T> HandleStorage<T>::Allocate()
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
				return Handle<T>();
			}
		}

		// インデックス取得
		Engine::Resource::Index _idx = m_indexQueue.front();
		m_indexQueue.pop();

		// ハンドル作成
		Handle<T> _handle(_idx, m_genVec[_idx]);
		return _handle;
	}
	template<typename T>
	inline void HandleStorage<T>::Remove(const Handle<T>& a_handle)
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
	inline bool HandleStorage<T>::IsValid(const Handle<T>& a_handle) const
	{
		// 世代が一致しているかどうか
		if (m_genVec.size() <= a_handle.GetIndex())
		{
			return false;
		}
		if(m_genVec[a_handle.GetIndex()] != a_handle.GetGeneration())
		{
			// 世代が一致していない = 無効
			return false;
		}

		// 世代が一致している = 有効
		return true;
	}
}