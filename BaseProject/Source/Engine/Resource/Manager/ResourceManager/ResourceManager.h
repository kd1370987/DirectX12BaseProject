#pragma once

#include "ResourcePool/ResourcePool.h"

namespace Engine::D3D12
{
	class CommandAllocator;
	class CommandList;
	class Fence;
}

namespace Engine::Resource
{
	// リソースの管理のみ
	class ResourceManager
	{
	public:

		// 初期化
		void Init(ID3D12Device* a_pDevice,ID3D12CommandQueue* a_copyQueue);

		// 更新
		void Update();

		// D3D
		D3D12::CommandList* GetCmdList();	// コマンドリスト取得
		void CmdQueueReset();				// キューのリセット
		void SignalFence(ID3D12CommandQueue* a_pCmdQueue);			// フェンスにシグナルを送る
		void WaitRender();
		void RegisterUploadBuffer(ID3D12Resource* a_pUploadBuffer);	// アップロードバッファのキャッシュ

		// リソースの追加
		template<typename T>
		Handle<T> Add(T&& a_resource);

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
		ResourcePool<Model>			m_modelPool;			// モデル
		ResourcePool<Texture>		m_texturePool;			// テクスチャ
		ResourcePool<Shader>		m_shaderPool;			// シェーダー
		ResourcePool<ShaderLibrary>	m_shaderLibraryPool;	//シェーダーライブラリ

		// リソース用D3D12オブジェクト群
		ID3D12CommandQueue* m_pCopyCmdQueue = nullptr;
		std::unique_ptr<D3D12::CommandAllocator> m_upCmdAllocator = nullptr;
		std::unique_ptr<D3D12::CommandList> m_upCmdList = nullptr;
		std::unique_ptr<D3D12::Fence> m_upFence = nullptr;
		UINT64 m_fenceValue = 0;
		HANDLE m_fenceEvent;

		// GPUに送信中かどうか
		bool m_isUploading = false;

		// アップロードバッファの解放待ちキュー
		std::vector<ID3D12Resource*> m_uploadBufferVec = {};


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
	template<typename T>
	inline Handle<T> ResourceManager::Add(T&& a_resource)
	{
		return RefPool<T>().Add(std::move(a_resource));
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
	template<> inline const ResourcePool<ShaderLibrary>& ResourceManager::GetPool<ShaderLibrary>() const { return m_shaderLibraryPool; }

	// テンプレート明示特殊化
	template<> inline ResourcePool<Model>& ResourceManager::RefPool<Model>() { return m_modelPool; }
	template<> inline ResourcePool<Texture>& ResourceManager::RefPool<Texture>() { return m_texturePool; }
	template<> inline ResourcePool<Shader>& ResourceManager::RefPool<Shader>() { return m_shaderPool; }
	template<> inline ResourcePool<ShaderLibrary>& ResourceManager::RefPool<ShaderLibrary>() { return m_shaderLibraryPool; }
}