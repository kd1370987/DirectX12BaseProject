#pragma once
namespace Engine::Resource
{
	// 各リソースのプール
	template<typename T>
	class ResourcePool
	{
	public:

		// リソースの追加
		Handle<T> Add(const T& a_resource);

		// リソースの消去
		void Clear();
		void Remove(const Handle<T>& a_handle);

		// リソースの取得
		const T* Get(const Handle<T>& a_handle)const;
		T* Ref(const Handle<T>& a_handle);
		const std::vector<T>& GetAll() const;
		std::vector<T>& RefAll();

		// ハンドルが有効かどうか
		bool IsValid(const Handle<T>& a_handle)const;
		
		// リソースサイズの前確保
		void Reserve(size_t a_capacity);

	private:
		Storage::HandleStorage<T> m_handleStorage = {};		// ハンドル管理
		std::vector<T> m_data;								// 実際のデータ
	};

	template<typename T>
	inline Handle<T> ResourcePool<T>::Add(const T& a_resource)
	{
		// ハンドル発行
		Handle<T> _handle = m_handleStorage.Allocate();
		if (_handle.idx >= m_data.size())
		{
			m_data.resize(_handle.idx + 1);
		}
		m_data[_handle.idx] = a_resource;

		return _handle;
	}
	template<typename T>
	inline void ResourcePool<T>::Clear()
	{
		m_handleStorage = {};
		m_data = {};
	}
	template<typename T>
	inline void ResourcePool<T>::Remove(const Handle<T>& a_handle)
	{
		m_handleStorage.Remove(a_handle);

		m_data[a_handle.idx] = {};
	}
	template<typename T>
	inline const T* ResourcePool<T>::Get(const Handle<T>&a_handle) const
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return &m_data[a_handle.idx];
		}
		return nullptr;
	}
	template<typename T>
	inline T* ResourcePool<T>::Ref(const Handle<T>& a_handle)
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return &m_data[a_handle.idx];
		}
		return nullptr;
	}
	template<typename T>
	inline const std::vector<T>& ResourcePool<T>::GetAll() const
	{
		return m_data;
	}
	template<typename T>
	inline std::vector<T>& ResourcePool<T>::RefAll()
	{
		return m_data;
	}
	template<typename T>
	inline bool ResourcePool<T>::IsValid(const Handle<T>& a_handle) const
	{
		return m_handleStorage.IsValid(a_handle);
	}
	template<typename T>
	inline void ResourcePool<T>::Reserve(size_t a_capacity)
	{
		m_handleStorage.Create(a_capacity);
		m_data.reserve(a_capacity);
	}
}