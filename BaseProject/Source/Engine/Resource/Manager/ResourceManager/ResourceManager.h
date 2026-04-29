#pragma once

#include "ResourcePool/ResourcePool.h"

namespace Engine::Resource
{
	// リソースの管理のみ
	class ResourceManager
	{
	public:

		// リソースの追加
		template<typename T>
		Handle<T> Add(const T& a_resource);

		// リソースの削除
		template<typename T>
		void Remove(const Handle<T>& a_handle);


		// リソースの取得
		template<typename T>
		const T* Get(const Handle<T>& a_handle)const;
		template<typename T>
		T* Ref(const Handle<T>& a_handle);

		// プールの取得
		template<typename T>
		const ResourcePool<T>& GetPool() const;
		template<typename T>
		ResourcePool<T>& RefPool();

	private:

		// 各リソースの実体プール
		ResourcePool<Model>		m_modelPool;
		ResourcePool<Texture>	m_texturePool;
		ResourcePool<Shader>	m_shaderPool;

	// シングルトン
	private:

		ResourceManager() = default;
		~ResourceManager() = default;

	public:
		static ResourceManager& Instance()
		{
			static ResourceManager _instance;
			return _instance;
		}
	};
	template<typename T>
	inline Handle<T> ResourceManager::Add(const T& a_resource)
	{
		return RefPool<T>().Add(a_resource);
	}
	template<typename T>
	inline void ResourceManager::Remove(const Handle<T>& a_handle)
	{
		return RefPool<T>().Remove(a_handle);
	}
	template<typename T>
	inline const T* ResourceManager::Get(const Handle<T>&a_handle) const
	{
		return GetPool<T>().Get(a_handle);
	}
	template<typename T>
	inline T* ResourceManager::Ref(const Handle<T>& a_handle)
	{
		return RefPool<T>().Ref(a_handle);
	}

	// プールの取得
	template<> inline const ResourcePool<Model>& ResourceManager::GetPool<Model>() const { return m_modelPool; }
	template<> inline const ResourcePool<Texture>& ResourceManager::GetPool<Texture>() const { return m_texturePool; }
	template<> inline const ResourcePool<Shader>& ResourceManager::GetPool<Shader>() const { return m_shaderPool; }

	// テンプレート明示特殊化
	template<> inline ResourcePool<Model>& ResourceManager::RefPool<Model>() { return m_modelPool; }
	template<> inline ResourcePool<Texture>& ResourceManager::RefPool<Texture>() { return m_texturePool; }
	template<> inline ResourcePool<Shader>& ResourceManager::RefPool<Shader>() { return m_shaderPool; }
}