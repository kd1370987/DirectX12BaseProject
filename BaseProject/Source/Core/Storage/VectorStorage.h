#pragma once

namespace StorageCommon
{
	using DataIndex = uint16_t;
	using DataGeneration = uint16_t;
	using ID = uint32_t;

	namespace Limits
	{
		constexpr ID MAX_STORAGE = 0xFFFFFFFF;
	}
}

template<typename T>
class VectorStorage
{
public:

	struct Data
	{
		T data;
		StorageCommon::DataGeneration gen = 0;
		bool isAlive = false;
	};

	void Init(UINT a_maxCount)
	{
		// オーバーフローチェック
		assert(a_maxCount <= std::numeric_limits<StorageCommon::DataIndex>::max());

		for (StorageCommon::DataIndex _idx = 0; _idx < static_cast<StorageCommon::DataIndex>(a_maxCount); ++_idx)
		{
			m_indexQueue.push(_idx);
		}

		m_dataVec.resize(a_maxCount);
		m_toString.resize(a_maxCount);
	}

	const T* Get(const StorageCommon::ID& a_id) const
	{	
		StorageCommon::DataIndex _idx = GetIndex(a_id);
		StorageCommon::DataGeneration _gen = GetGeneration(a_id);

		if (_idx >= m_dataVec.size())
		{
			return nullptr;
		}

		const Data& _slot = m_dataVec[_idx];
		if (!_slot.isAlive)
		{
			return nullptr;
		}
		if (_slot.gen != _gen)
		{
			return nullptr;
		}

		return &_slot.data;
	}

	const T* Get(const std::string& a_key) const
	{
		auto _it = m_idMap.find(a_key);
		if (_it == m_idMap.end())
		{
			return nullptr;
		}
		
		return Get(_it->second);
	}

	/// <summary>
	/// キーからIDを取得
	/// </summary>
	/// <param name="a_key">文字列</param>
	/// <returns>失敗したときはMAX_STORAGRが帰る</returns>
	StorageCommon::ID GetID(const std::string& a_key);

	StorageCommon::ID Add(const std::string& a_key, T&& a_data)
	{
		if (m_indexQueue.empty())
		{
			assert(0 && "ストレージの設定上限に達しました");
			return StorageCommon::Limits::MAX_STORAGE;
		}


		auto _id = GetID(a_key);
		if (_id == StorageCommon::Limits::MAX_STORAGE)
		{	
			StorageCommon::DataIndex _idx =m_indexQueue.front();
			m_indexQueue.pop();

			// 既存のストレージに上書き
			m_toString[_idx] = a_key;
			Data& _data = m_dataVec[_idx];
			_data.data = std::move(a_data);
			_data.isAlive = true;
			StorageCommon::ID _newId = CreateID(_idx,_data.gen);
			m_idMap.emplace(a_key,_newId);

			return _newId;
		}
		else
		{
			StorageCommon::DataIndex _idx = GetIndex(_id);
			m_dataVec[_idx].data = std::move(a_data);
			return _id;
		}
	}

	void Destroy(const StorageCommon::ID& a_id)
	{
		StorageCommon::DataIndex _idx = GetIndex(a_id);
		StorageCommon::DataGeneration _gen = GetGeneration(a_id);

		Data& _data = m_dataVec[_idx];
		if (!_data.isAlive)
		{
			return;
		}
		if (_data.gen != _gen)
		{
			return;
		}

		_data.gen++;
		_data.isAlive = false;

		auto _str = m_toString[_idx];
		m_idMap.erase(_str);
		m_toString[_idx] = "";

		// 未使用キューにインデックスを戻す
		m_indexQueue.push(_idx);
	}

	/// <summary>
	/// IDの有効性チェック
	/// </summary>
	/// <param name="a_id"></param>
	/// <returns></returns>
	bool IsValid(const StorageCommon::ID& a_id);

private:

	StorageCommon::ID CreateID(
		StorageCommon::DataIndex a_idx,
		StorageCommon::DataGeneration a_gen
	)
	{
		return (StorageCommon::ID(a_gen) << 16) | a_idx;
	}

	StorageCommon::DataGeneration GetGeneration(StorageCommon::ID a_id)
	{
		return uint16_t(a_id >> 16);
	}

	StorageCommon::DataIndex GetIndex(StorageCommon::ID a_id)
	{
		return uint16_t(a_id & 0xFFFF);
	}

private:

	std::unordered_map<std::string, StorageCommon::ID> m_idMap;
	std::vector<std::string> m_toString;
	std::vector<Data> m_dataVec;
	

	std::queue<StorageCommon::DataIndex> m_indexQueue;
};

template<typename T>
inline StorageCommon::ID VectorStorage<T>::GetID(const std::string& a_key)
{
	// 名前でIDを検索
	auto _it = m_idMap.find(a_key);
	if (_it == m_idMap.end())
	{
		// ないとき
		return StorageCommon::Limits::MAX_STORAGE;
	}

	// あったときは返す
	return _it->second;
}

template<typename T>
inline bool VectorStorage<T>::IsValid(const StorageCommon::ID& a_id)
{
	return a_id != StorageCommon::Limits::MAX_STORAGE;
}
