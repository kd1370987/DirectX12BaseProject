#pragma once

// アセットデータベース
#include "../AssetDatabase/AssetDatabase.h"

// 各リソース
#include "../../Data/StateMachineAsset/StateMachineAsset.h"
#include "../../Data/Particles/ParticlesAsset.h"

// ローダー
#include "../../Loader/DefaultLoader.h"

namespace Engine::Resource
{
	// リソースの管理のみ
	class ResourceManager
	{
	public:

		// 解放
		void Release();

		// リソースの読み込み
		template<typename T>
		inline Handle<T> Load(const Engine::GUID& a_guid);

		// リソースの追加
		template<typename T>
		Handle<T> Add(T&& a_resource);

		/// <summary>
		/// キャッシュにも追加されるアセットのデータ追加用関数
		/// </summary>
		/// <typeparam name="T">型</typeparam>
		/// <param name="a_resource">リソースの実態(std::move()必須)</param>
		/// <param name="a_guid">アセットの読み込み時GUID</param>
		/// <returns>保存されたラインタイムハンドル</returns>
		template<typename T>
		Handle<T> AddResourceAndGUID(T&& a_resource,const Engine::GUID& a_guid);

		// リソースの削除
		template<typename T>
		void Remove(const Handle<T>& a_handle);


		// リソースの取得
		template<typename T>
		const T* Get(const Handle<T>& a_handle)const;
		template<typename T>
		T* Ref(const Handle<T>& a_handle);
		template<typename T>
		const T* Accece(const Handle<T>& a_handle);
		template<typename T>
		const T* Accece(const uint16_t& a_index);

		// プールの取得
		template<typename T>
		const Pool::ItemPool<T>& GetPool() const;
		template<typename T>
		Pool::ItemPool<T>& RefPool();

		// キャッシュアクセス
		template<typename T>
		const Handle<T>& GetCache(const Engine::GUID& a_guid);
		template<typename T>
		const Engine::GUID& GetCache(const Handle<T>& a_handle);

		// キャッシュから削除
		template<typename T>
		void RemoveCache(const Handle<T>& a_handle);
		template<typename T>
		void RemoveCache(const Engine::GUID& a_guid);

		// すでに読み込まれているかのチェック
		template<typename T>
		bool Has(const Engine::GUID& a_guid);

		// ハンドルの有効チェック
		template<typename T>
		bool IsValiad(const Handle<T>& a_handle);

	private:

		// キャッシュ追加
		template<typename T>
		void RegisterCache(const Handle<T>& a_handle, const Engine::GUID& a_guid);

		// キャッシュのマップ参照
		template<typename T>
		std::unordered_map<Engine::GUID, Handle<T>>& RefMap();

	private:

		// 各リソースの実体プール
		Pool::ItemPool<Model>			m_modelPool;			// モデル
		std::unordered_map<Engine::GUID, Handle<Model>> m_modelCache = {};

		Pool::ItemPool<Material>		m_materialPool;			// マテリアル
		std::unordered_map<Engine::GUID, Handle<Material>> m_materialCache = {};

		Pool::ItemPool<Mesh>			m_meshPool;				// メッシュ
		std::unordered_map<Engine::GUID, Handle<Mesh>> m_meshCache = {};

		Pool::ItemPool<AnimationData>	m_animationPool;		// アニメーション
		std::unordered_map<Engine::GUID, Handle<AnimationData>> m_animationCache = {};

		Pool::ItemPool<Texture>			m_texturePool;			// テクスチャ
		std::unordered_map<Engine::GUID, Handle<Texture>> m_texCache = {};

		Pool::ItemPool<Shader>			m_shaderPool;			// シェーダー
		std::unordered_map<Engine::GUID, Handle<Shader>> m_shaderCache = {};

		Pool::ItemPool<ShaderLibrary>	m_shaderLibraryPool;	// シェーダーライブラリ
		std::unordered_map<Engine::GUID, Handle<ShaderLibrary>> m_shaderLibraryCache = {};

		Pool::ItemPool<StateMachineAsset> m_stateMachinePool;	// ステートマシン
		std::unordered_map<Engine::GUID, Handle<StateMachineAsset>> m_stateMachineAssetCache = {};

		Pool::ItemPool<ParticlesAsset> m_particlesAssetPool;	// ステートマシン
		std::unordered_map<Engine::GUID, Handle<ParticlesAsset>> m_particleCache = {};


	// シングルトン
	private:

		ResourceManager();
		~ResourceManager();

	public:
		static ResourceManager& Instance()
		{
			static ResourceManager _instance;
			return _instance;
		}
	};
	// リソースのロード
	template<typename T>
	inline Handle<T> ResourceManager::Load(const Engine::GUID& a_guid)
	{
		// 読み込み済みかチェック
		const auto& _handle = GetCache<T>(a_guid);
		if (_handle != Handle<T>()) return _handle;

		// パス取得
		std::string _filePath = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);

		// リソースのビルド
		T _resourceData = DefaultLoader<T>::LoadFromFile(_filePath);

		// プールに登録してハンドルを発行
		return AddResourceAndGUID(std::move(_resourceData),a_guid);
	}
	// リソースの追加
	template<typename T>
	inline Handle<T> ResourceManager::Add(T&& a_resource)
	{
		return RefPool<T>().Add(std::move(a_resource));
	}
	template<typename T>
	inline Handle<T> ResourceManager::AddResourceAndGUID(T&& a_resource, const Engine::GUID& a_guid)
	{
		auto _handle = RefPool<T>().Add(std::move(a_resource));
		RegisterCache<T>(_handle, a_guid); // キャッシュにも登録
		return _handle;                    // ハンドルを返す
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

	template<typename T>
	inline const T* ResourceManager::Accece(const Handle<T>& a_handle)
	{
		return GetPool<T>().Access(a_handle.GetIndex());
	}


	template<typename T>
	inline const T* ResourceManager::Accece(const uint16_t& a_index)
	{
		return GetPool<T>().Access(a_index);
	}

	// テンプレート明示特殊化
	// プールの取得
	template<> inline const Pool::ItemPool<Model>& ResourceManager::GetPool<Model>() const							{ return m_modelPool; }
	template<> inline const Pool::ItemPool<Material>& ResourceManager::GetPool<Material>() const					{ return m_materialPool; }
	template<> inline const Pool::ItemPool<Mesh>& ResourceManager::GetPool<Mesh>() const							{ return m_meshPool; }
	template<> inline const Pool::ItemPool<AnimationData>& ResourceManager::GetPool<AnimationData>() const			{ return m_animationPool; }
	template<> inline const Pool::ItemPool<Texture>& ResourceManager::GetPool<Texture>() const						{ return m_texturePool; }
	template<> inline const Pool::ItemPool<Shader>& ResourceManager::GetPool<Shader>() const						{ return m_shaderPool; }
	template<> inline const Pool::ItemPool<ShaderLibrary>& ResourceManager::GetPool<ShaderLibrary>() const			{ return m_shaderLibraryPool; }
	template<> inline const Pool::ItemPool<StateMachineAsset>& ResourceManager::GetPool<StateMachineAsset>() const	{ return m_stateMachinePool; }
	template<> inline const Pool::ItemPool<ParticlesAsset>& ResourceManager::GetPool<ParticlesAsset>() const		{ return m_particlesAssetPool; }
	
	// テンプレート明示特殊化
	// プールの参照
	template<> inline Pool::ItemPool<Model>& ResourceManager::RefPool<Model>()							{ return m_modelPool; }
	template<> inline Pool::ItemPool<Material>& ResourceManager::RefPool<Material>()					{ return m_materialPool; }
	template<> inline Pool::ItemPool<Mesh>& ResourceManager::RefPool<Mesh>()							{ return m_meshPool; }
	template<> inline Pool::ItemPool<AnimationData>& ResourceManager::RefPool<AnimationData>()			{ return m_animationPool; }
	template<> inline Pool::ItemPool<Texture>& ResourceManager::RefPool<Texture>()						{ return m_texturePool; }
	template<> inline Pool::ItemPool<Shader>& ResourceManager::RefPool<Shader>()						{ return m_shaderPool; }
	template<> inline Pool::ItemPool<ShaderLibrary>& ResourceManager::RefPool<ShaderLibrary>()			{ return m_shaderLibraryPool; }
	template<> inline Pool::ItemPool<StateMachineAsset>& ResourceManager::RefPool<StateMachineAsset>()	{ return m_stateMachinePool; }
	template<> inline Pool::ItemPool<ParticlesAsset>& ResourceManager::RefPool<ParticlesAsset>()		{ return m_particlesAssetPool; }

	// キャッシュアクセス
	template<typename T>
	inline const Handle<T>& ResourceManager::GetCache(const Engine::GUID& a_guid)
	{
		auto& _map = RefMap<T>();
		auto _it = _map.find(a_guid);
		if (_it != _map.end())
		{
			return _it->second;
		}
		ENGINE_LOG("登録されていないGUIDです");
		static Handle<T> s_defaultHandle = {};
		return s_defaultHandle;
	}
	template<typename T>
	inline const Engine::GUID& ResourceManager::GetCache(const Handle<T>& a_handle)
	{
		auto& _map = RefMap<T>();
		for (const auto& [_guid, _h] : _map)
		{
			if (_h == a_handle)
			{
				return _guid;
			}
		}
		ENGINE_LOG("登録されていないハンドルです");
		static Engine::GUID s_defaultGuid = {};
		return s_defaultGuid;
	}
	// キャッシュ削除
	template<typename T>
	inline void ResourceManager::RemoveCache(const Handle<T>& a_handle)
	{
		auto& _map = RefMap<T>();
		for (auto _it = _map.begin(); _it != _map.end(); ++_it)
		{
			if (_it->second == a_handle)
			{
				_map.erase(_it); // ハンドルが一致したものを消す
				break;
			}
		}
	}

	template<typename T>
	inline void ResourceManager::RemoveCache(const Engine::GUID& a_guid)
	{
		RefMap<T>().erase(a_guid); // GUIDから一発で削除
	}

	template<typename T>
	inline bool ResourceManager::Has(const Engine::GUID& a_guid)
	{
		auto& _map = RefMap<T>();

		auto _it = _map.find(a_guid);
		if (_it != _map.end())
		{
			return true;
		}
		return false;
	}

	template<typename T>
	inline bool ResourceManager::IsValiad(const Handle<T>& a_handle)
	{
		auto& _map = RefMap<T>();
		for (auto& [_guid,_handle] : _map)
		{
			if (_handle == a_handle)
			{
				return true;
			}
		}
		return false;
	}

	// 型ごとにキャッシュに登録
	template<typename T>
	inline void ResourceManager::RegisterCache(const Handle<T>& a_handle, const Engine::GUID& a_guid)
	{
		RefMap<T>()[a_guid] = a_handle;
	}

	// テンプレート明示特殊化
	// マップの参照
	template<> inline std::unordered_map<Engine::GUID, Handle<Model>>& ResourceManager::RefMap<Model>() { return m_modelCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<Material>>& ResourceManager::RefMap<Material>() { return m_materialCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<Mesh>>& ResourceManager::RefMap<Mesh>() { return m_meshCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<AnimationData>>& ResourceManager::RefMap<AnimationData>() { return m_animationCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<Texture>>& ResourceManager::RefMap<Texture>() { return m_texCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<Shader>>& ResourceManager::RefMap<Shader>() { return m_shaderCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<ShaderLibrary>>& ResourceManager::RefMap<ShaderLibrary>() { return m_shaderLibraryCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<StateMachineAsset>>& ResourceManager::RefMap<StateMachineAsset>() { return m_stateMachineAssetCache; }
	template<> inline std::unordered_map<Engine::GUID, Handle<ParticlesAsset>>& ResourceManager::RefMap<ParticlesAsset>() { return m_particleCache; }
}
