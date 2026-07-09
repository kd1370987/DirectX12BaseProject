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
	template<typename T>
	struct ResourceData
	{
		Pool::ItemPool<T>							pool;					// モデル
		std::unordered_map<Engine::GUID, Handle<T>> cache = {};				// GUID to Handle
		std::vector<uint16_t>						manualRefCounts = {};	// ECS外の処理カウント
		std::vector<uint16_t>						ecsRefCounts = {};		// ECS走査時のカウント
	};

	// リソースの管理のみ
	class ResourceManager
	{
	public:

		// 解放
		void Release();

		// リソースの読み込み
		template<typename T>
		inline ResourceRef<T> Load(const Engine::GUID& a_guid);

		// リソースの追加
		template<typename T>
		ResourceRef<T> Add(T&& a_resource);

		template<typename T>
		void AddRef(const Handle<T>& a_handle);

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

		template<typename T>
		void ReleaseRef(const Handle<T>& a_handle);

		// リソースの取得
		template<typename T>
		const T* Get(const Handle<T>& a_handle)const;
		template<typename T>
		const T* Get(const ResourceRef<T>& a_handle)const;
		template<typename T>
		T* Ref(const Handle<T>& a_handle);
		template<typename T>
		T* Ref(const ResourceRef<T>& a_handle);
		template<typename T>
		const T* Accece(const Handle<T>& a_handle);
		template<typename T>
		const T* Accece(const ResourceRef<T>& a_handle);
		template<typename T>
		const T* Accece(const uint16_t& a_index);

		// リソースの解放
		template<typename T>
		void SweepUnused();

		// 疑似ガベージコレクション : 全プールのスイープ処理を実行
		// 参照カウントがないプールのリソースは解放される
		void RunGarbageCollectionSweep();

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
		bool IsValid(const Handle<T>& a_handle);

	private:

		// キャッシュ追加
		template<typename T>
		void RegisterCache(const Handle<T>& a_handle, const Engine::GUID& a_guid);

		// キャッシュのマップ参照
		template<typename T>
		std::unordered_map<Engine::GUID, Handle<T>>& RefMap();

		/// <summary>
		/// テンプレート特殊化された内部データを参照するための関数
		/// </summary>
		/// <typeparam name="T">型情報</typeparam>
		/// <returns>型に紐づく情報</returns>
		template<typename T>
		const ResourceData<T>& GetData() const;

		/// <summary>
		/// テンプレート特殊化された内部データを参照するための関数
		/// </summary>
		/// <typeparam name="T">型情報</typeparam>
		/// <returns>型に紐づく情報</returns>
		template<typename T>
		ResourceData<T>& RefData();

	private:

		// 各リソースの実体プール
		ResourceData<Model> m_modelData;						// モデル
		ResourceData<Material> m_materialData;					// マテリアル
		ResourceData<Mesh> m_meshData;							// メッシュ
		ResourceData<AnimationData> m_animationData;			// アニメーション
		ResourceData<Texture> m_textureData;					// テクスチャ
		ResourceData<Shader> m_shaderData;						// シェーダー
		ResourceData<ShaderLibrary> m_shaderLibraryData;		// シェーダーライブラリ
		ResourceData<StateMachineAsset> m_stateMachineAssetData;// ステートマシンアセット
		ResourceData<ParticlesAsset> m_particleAssetData;		// パーティクル
		ResourceData<ShadingModelTable> m_shadingModelTableData;// シェーディングモデルテーブル

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
	inline ResourceRef<T> ResourceManager::Load(const Engine::GUID& a_guid)
	{
		// 読み込み済みかチェック
		const auto& _handle = GetCache<T>(a_guid);
		if (IsValid(_handle)) return ResourceRef<T>(_handle);

		std::string _filePath = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);	// パス取得
		T _resourceData = DefaultLoader<T>::LoadFromFile(_filePath);					// リソースのビルド

		// プールに登録してハンドルを発行
		return ResourceRef<T>(AddResourceAndGUID(std::move(_resourceData), a_guid));
	}
	// リソースの追加
	template<typename T>
	inline ResourceRef<T> ResourceManager::Add(T&& a_resource)
	{
		return ResourceRef<T>(RefPool<T>().Add(std::move(a_resource)));
	}
	template<typename T>
	inline void ResourceManager::AddRef(const Handle<T>& a_handle)
	{
		auto& _data = RefData<T>();
		uint16_t _idx = a_handle.GetIndex();
		if (_data.manualRefCounts.size() <= _idx)
		{
			_data.manualRefCounts.resize(_idx + 1, 0);
		}
		_data.manualRefCounts[_idx]++;
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
	inline void ResourceManager::ReleaseRef(const Handle<T>& a_handle)
	{
		auto& _data = RefData<T>();
		uint16_t _idx = a_handle.GetIndex();
		if (_idx < _data.manualRefCounts.size() && _data.manualRefCounts[_idx] > 0)
		{
			_data.manualRefCounts[_idx]--;
		}
	}
	template<typename T>
	inline const T* ResourceManager::Get(const Handle<T>&a_handle) const
	{
		return GetPool<T>().Get(a_handle);
	}
	template<typename T>
	inline const T* ResourceManager::Get(const ResourceRef<T>& a_handle) const
	{
		return Get(a_handle.GetRaw());
	}
	template<typename T>
	inline T* ResourceManager::Ref(const Handle<T>& a_handle)
	{
		return RefPool<T>().Ref(a_handle);
	}

	template<typename T>
	inline T* ResourceManager::Ref(const ResourceRef<T>& a_handle)
	{
		return Ref(a_handle.GetRaw());
	}

	template<typename T>
	inline const T* ResourceManager::Accece(const Handle<T>& a_handle)
	{
		return GetPool<T>().Access(a_handle.GetIndex());
	}

	template<typename T>
	inline const T* ResourceManager::Accece(const ResourceRef<T>& a_handle)
	{
		return Accece(a_handle.GetRaw());
	}


	template<typename T>
	inline const T* ResourceManager::Accece(const uint16_t& a_index)
	{
		return GetPool<T>().Access(a_index);
	}

	template<typename T>
	inline void ResourceManager::SweepUnused()
	{
		auto& _data = RefData<T>();

		// manual と ecs の配列で大きい方のサイズまで走査
		size_t _maxSize = std::max(_data.manualRefCounts.size(), _data.ecsRefCounts.size());

		for (uint16_t i = 0; i < _maxSize; ++i)
		{
			uint16_t _manual = (i < _data.manualRefCounts.size()) ? _data.manualRefCounts[i] : 0;
			uint16_t _ecs = (i < _data.ecsRefCounts.size()) ? _data.ecsRefCounts[i] : 0;

			// アプリ側からも、ECS側からも参照されていない場合
			if (_manual == 0 && _ecs == 0)
			{
				// ItemPool内に実体が存在しているかチェック
				if (_data.pool.Access(i) != nullptr)
				{
					// インデックスと世代から正しいハンドルを復元する
					uint16_t _generation = _data.pool.GetGeneration(i);
					Handle<T> _targetHandle(i, _generation);

					// キャッシュ (GUIDマップ) から削除
					for (auto it = _data.cache.begin(); it != _data.cache.end(); ) {
						if (it->second == _targetHandle) {
							it = _data.cache.erase(it);
						}
						else {
							++it;
						}
					}

					// プールの Remove を呼んで実体とキューを安全に解放
					_data.pool.Remove(_targetHandle);
				}
			}
		}
	}

	// プールの取得
	template<typename T>
	inline const Pool::ItemPool<T>& ResourceManager::GetPool() const
	{
		return GetData<T>().pool;
	}

	// プールの参照
	template<typename T>
	inline Pool::ItemPool<T>& ResourceManager::RefPool()
	{
		return RefData<T>().pool;
	}

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

	// ハンドルが使用可能かどうか
	template<typename T>
	inline bool ResourceManager::IsValid(const Handle<T>& a_handle)
	{
		return GetPool<T>().IsValid(a_handle);
	}

	// 型ごとにキャッシュに登録
	template<typename T>
	inline void ResourceManager::RegisterCache(const Handle<T>& a_handle, const Engine::GUID& a_guid)
	{
		RefMap<T>()[a_guid] = a_handle;
	}

	// マップの参照
	template<typename T>
	inline std::unordered_map<Engine::GUID, Handle<T>>& ResourceManager::RefMap()
	{
		return RefData<T>().cache;
	}

	// プールの参照
	template<> inline ResourceData<Model>& ResourceManager::RefData<Model>() { return  m_modelData; }
	template<> inline ResourceData<Material>& ResourceManager::RefData<Material>() { return m_materialData; }
	template<> inline ResourceData<Mesh>& ResourceManager::RefData<Mesh>() { return m_meshData; }
	template<> inline ResourceData<AnimationData>& ResourceManager::RefData<AnimationData>() { return m_animationData; }
	template<> inline ResourceData<Texture>& ResourceManager::RefData<Texture>() { return m_textureData; }
	template<> inline ResourceData<Shader>& ResourceManager::RefData<Shader>() { return m_shaderData; }
	template<> inline ResourceData<ShaderLibrary>& ResourceManager::RefData<ShaderLibrary>() { return m_shaderLibraryData; }
	template<> inline ResourceData<StateMachineAsset>& ResourceManager::RefData<StateMachineAsset>() { return m_stateMachineAssetData; }
	template<> inline ResourceData<ParticlesAsset>& ResourceManager::RefData<ParticlesAsset>() { return m_particleAssetData; }
	template<> inline ResourceData<ShadingModelTable>& ResourceManager::RefData<ShadingModelTable>() { return m_shadingModelTableData; }

	// プールの取得
	template<> inline const ResourceData<Model>& ResourceManager::GetData<Model>() const { return  m_modelData; }
	template<> inline const ResourceData<Material>& ResourceManager::GetData<Material>() const { return m_materialData; }
	template<> inline const ResourceData<Mesh>& ResourceManager::GetData<Mesh>() const { return m_meshData; }
	template<> inline const ResourceData<AnimationData>& ResourceManager::GetData<AnimationData>() const { return m_animationData; }
	template<> inline const ResourceData<Texture>& ResourceManager::GetData<Texture>() const { return m_textureData; }
	template<> inline const ResourceData<Shader>& ResourceManager::GetData<Shader>() const { return m_shaderData; }
	template<> inline const ResourceData<ShaderLibrary>& ResourceManager::GetData<ShaderLibrary>() const { return m_shaderLibraryData; }
	template<> inline const ResourceData<StateMachineAsset>& ResourceManager::GetData<StateMachineAsset>() const { return m_stateMachineAssetData; }
	template<> inline const ResourceData<ParticlesAsset>& ResourceManager::GetData<ParticlesAsset>() const { return m_particleAssetData; }
	template<> inline const ResourceData<ShadingModelTable>& ResourceManager::GetData<ShadingModelTable>() const { return m_shadingModelTableData; }
}

namespace Engine
{
	// =========================================================
	// ResourceRef の中身を実装
	// リソースマネージャーの実装が見えている必要があるため
	// =========================================================

	// コンストラクタ
	template<typename T>
	inline ResourceRef<T>::ResourceRef(Handle<T> a_h) : m_handle(a_h) 
	{
		if (Resource::ResourceManager::Instance().IsValid(m_handle))
		{
			Resource::ResourceManager::Instance().AddRef(m_handle);
		}
	}

	// デストラクタ
	template<typename T>
	inline ResourceRef<T>::~ResourceRef() {
		if (Resource::ResourceManager::Instance().IsValid(m_handle)) 
		{
			Resource::ResourceManager::Instance().ReleaseRef(m_handle);
		}
	}

	// コピーコンストラクタ
	template<typename T>
	inline ResourceRef<T>::ResourceRef(const ResourceRef& a_other) : m_handle(a_other.m_handle)
	{
		if (Resource::ResourceManager::Instance().IsValid(m_handle)) 
		{
			Resource::ResourceManager::Instance().AddRef(m_handle);
		}
	}

	// コピー代入演算子 (超重要！)
	template<typename T>
	inline ResourceRef<T>& ResourceRef<T>::operator=(const ResourceRef& a_other)
	{
		if (this != &a_other) {
			// 古いもののカウントを減らし、新しいものを増やす
			if (Resource::ResourceManager::Instance().IsValid(m_handle))
			{
				Resource::ResourceManager::Instance().ReleaseRef(m_handle);
			}
			m_handle = a_other.m_handle;
			if (Resource::ResourceManager::Instance().IsValid(m_handle)) 
			{
				Resource::ResourceManager::Instance().AddRef(m_handle);
			}
		}
		return *this;
	}

	// ムーブコンストラクタ
	template<typename T>
	inline ResourceRef<T>::ResourceRef(ResourceRef&& a_other) noexcept : m_handle(a_other.m_handle)
	{
		a_other.m_handle = {}; // 元のハンドルを空にする
	}

	// ムーブ代入演算子
	template<typename T>
	inline ResourceRef<T>& ResourceRef<T>::operator=(ResourceRef&& a_other) noexcept
	{
		if (this != &a_other) {
			if (Resource::ResourceManager::Instance().IsValid(m_handle)) 
			{
				Resource::ResourceManager::Instance().ReleaseRef(m_handle);
			}
			m_handle = a_other.m_handle;
			a_other.m_handle = {};
		}
		return *this;
	}
}
