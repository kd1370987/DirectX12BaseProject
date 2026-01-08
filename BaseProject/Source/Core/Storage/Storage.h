#pragma once

template<typename Key,typename Data>
class Storage
{
public:

	Storage() = default;
	~Storage() = default;

	// データの追加
	void Add(const Key& a_key, const std::shared_ptr<Data>& a_data)
	{
		m_storageMap[a_key] = a_data;
	}

	void Add(const Key& a_key, const Data& a_data)
	{
		auto _spData = std::make_shared<Data>();
		m_storageMap[a_key] = _spData;
	}

	// データの取得
	std::shared_ptr<Data> Get(const Key& a_key)
	{
		auto _it = m_storageMap.find(a_key);
		if (_it != m_storageMap.end())
		{
			return _it->second;
		}
		return nullptr;
	}

private:
	std::unordered_map<Key, std::shared_ptr<Data>> m_storageMap;
};