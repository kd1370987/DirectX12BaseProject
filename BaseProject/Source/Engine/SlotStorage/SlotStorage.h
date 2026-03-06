#pragma once

template<typename T>
class SlotStorage
{
public:

	struct Data
	{
		std::shared_ptr<T> spData = nullptr;
		Engine::Resource::DataGeneration gen = 0;
		bool isAlive = false;
	};

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_maxCount">データの容量設定</param>
	void Init(UINT a_maxCount);

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	/// <summary>
	/// 登録IDで検索
	/// </summary>
	/// <param name="a_id">登録ID</param>
	/// <returns>データのポインタ</returns>
	const T* Get(const Engine::Resource::ID& a_id) const;

	T* Ref(const Engine::Resource::ID& a_id)const;

	/// <summary>
	/// 登録した時の文字列で検索
	/// </summary>
	/// <param name="a_key">文字列キー</param>
	/// <returns>データのポインタ</returns>
	const T* Get(const std::string& a_key) const;

	

	/// <summary>
	/// キーからIDを取得
	/// </summary>
	/// <param name="a_key">文字列</param>
	/// <returns>失敗したときはMAX_STORAGRが帰る</returns>
	Engine::Resource::ID GetID(const std::string& a_key);

	/// <summary>
	/// データの追加
	/// </summary>
	/// <param name="a_key">文字列キー</param>
	/// <param name="a_data">データの実態</param>
	/// <returns>保存したID</returns>
	Engine::Resource::ID Add(const std::string& a_key, std::shared_ptr<T> a_spData);

	/// <summary>
	/// データの削除
	/// </summary>
	/// <param name="a_id"></param>
	void Destroy(const Engine::Resource::ID& a_id);

	/// <summary>
	/// IDの有効性チェック
	/// </summary>
	/// <param name="a_id"></param>
	/// <returns></returns>
	bool IsValid(const Engine::Resource::ID& a_id);

	/// <summary>
	/// 登録されているかのチェック
	/// </summary>
	/// <param name="a_key">文字列キー</param>
	/// <returns>持っていたらture</returns>
	bool Has(const std::string& a_key);

private:

	Engine::Resource::ID CreateID(
		Engine::Resource::DataIndex a_idx,
		Engine::Resource::DataGeneration a_gen
	)
	{
		return (Engine::Resource::ID(a_gen) << 16) | a_idx;
	}

	/// <summary>
	/// 世代取得
	/// </summary>
	/// <param name="a_id">ID</param>
	/// <returns>IDから抽出した世代</returns>
	Engine::Resource::DataGeneration GetGeneration(Engine::Resource::ID a_id) const;

	/// <summary>
	/// インデックス取得
	/// </summary>
	/// <param name="a_id">ID</param>
	/// <returns>IDから取得したインデックス</returns>
	Engine::Resource::DataIndex GetIndex(Engine::Resource::ID a_id) const;

private:

	std::unordered_map<std::string, Engine::Resource::ID> m_idMap = {};
	std::vector<std::string> m_toString = {};
	std::vector<Data> m_dataVec = {};


	std::queue<Engine::Resource::DataIndex> m_indexQueue = {};
};

template<typename T>
inline void SlotStorage<T>::Init(UINT a_maxCount)
{
	// オーバーフローチェック
	assert(a_maxCount <= std::numeric_limits<Engine::Resource::DataIndex>::max());

	for (Engine::Resource::DataIndex _idx = 0; _idx < static_cast<Engine::Resource::DataIndex>(a_maxCount); ++_idx)
	{
		m_indexQueue.push(_idx);
	}

	m_dataVec.resize(a_maxCount);
	m_toString.resize(a_maxCount);
}

template<typename T>
inline void SlotStorage<T>::Release()
{
	m_idMap.clear();
	m_toString.clear();
	m_dataVec.clear();
	m_indexQueue.emplace();
}

template<typename T>
inline const T* SlotStorage<T>::Get(const Engine::Resource::ID& a_id) const
{
	Engine::Resource::DataIndex _idx = static_cast<Engine::Resource::DataIndex>(GetIndex(a_id));
	Engine::Resource::DataGeneration _gen = static_cast<Engine::Resource::DataGeneration>(GetGeneration(a_id));

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

	return _slot.spData.get();
}

template<typename T>
inline T* SlotStorage<T>::Ref(const Engine::Resource::ID& a_id) const
{
	Engine::Resource::DataIndex _idx = static_cast<Engine::Resource::DataIndex>(GetIndex(a_id));
	Engine::Resource::DataGeneration _gen = static_cast<Engine::Resource::DataGeneration>(GetGeneration(a_id));

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

	return _slot.spData.get();
}

template<typename T>
inline const T* SlotStorage<T>::Get(const std::string& a_key) const
{
	auto _it = m_idMap.find(a_key);
	if (_it == m_idMap.end())
	{
		return nullptr;
	}

	return Get(_it->second);
}

template<typename T>
inline Engine::Resource::ID SlotStorage<T>::GetID(const std::string& a_key)
{
	// 名前でIDを検索
	auto _it = m_idMap.find(a_key);
	if (_it == m_idMap.end())
	{
		// ないとき
		//assert(0 && "IDが見つかりません");
		return Engine::Resource::Limits::INVALID_ID;
	}

	// あったときは返す
	return _it->second;
}

template<typename T>
inline Engine::Resource::ID SlotStorage<T>::Add(const std::string& a_key, std::shared_ptr<T> a_spData)
{
	if (m_indexQueue.empty())
	{
		assert(0 && "ストレージの設定上限に達しました");
		return Engine::Resource::Limits::INVALID_ID;
	}


	auto _id = GetID(a_key);
	if (_id == Engine::Resource::Limits::INVALID_ID)
	{
		Engine::Resource::DataIndex _idx = m_indexQueue.front();
		m_indexQueue.pop();

		// 既存のストレージに上書き
		m_toString[_idx] = a_key;
		Data& _data = m_dataVec[_idx];
		_data.spData = a_spData;
		_data.isAlive = true;
		Engine::Resource::ID _newId = CreateID(_idx, _data.gen);
		m_idMap.emplace(a_key, _newId);

		return _newId;
	}
	else
	{
		Engine::Resource::DataIndex _idx = GetIndex(_id);
		m_dataVec[_idx].spData = a_spData;
		return _id;
	}
}

template<typename T>
inline void SlotStorage<T>::Destroy(const Engine::Resource::ID& a_id)
{
	Engine::Resource::DataIndex _idx = GetIndex(a_id);
	Engine::Resource::DataGeneration _gen = GetGeneration(a_id);

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

template<typename T>
inline bool SlotStorage<T>::IsValid(const Engine::Resource::ID& a_id)
{
	return a_id != Engine::Resource::Limits::INVALID_ID;
}

template<typename T>
inline bool SlotStorage<T>::Has(const std::string& a_key)
{
	return IsValid(GetID(a_key));
}

template<typename T>
inline Engine::Resource::DataGeneration SlotStorage<T>::GetGeneration(Engine::Resource::ID a_id) const
{
	return uint16_t(a_id >> 16);
}

template<typename T>
inline Engine::Resource::DataIndex SlotStorage<T>::GetIndex(Engine::Resource::ID a_id) const
{
	return static_cast<Engine::Resource::DataIndex>(uint16_t(a_id & 0xFFFF));
}
