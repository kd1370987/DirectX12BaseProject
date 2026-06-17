#pragma once

// 各リソース
#include "../../Data/StateMachineAsset/StateMachineAsset.h"

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

		// 解放
		void Release();

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
		template<typename T>
		const T* Accece(const Handle<T>& a_handle);
		template<typename T>
		const T* Accece(const uint16_t& a_index);

		// プールの取得
		template<typename T>
		const Pool::ItemPool<T>& GetPool() const;
		template<typename T>
		Pool::ItemPool<T>& RefPool();

	private:

		// 各リソースの実体プール
		Pool::ItemPool<Model>			m_modelPool;			// モデル
		Pool::ItemPool<Material>		m_materialPool;			// マテリアル
		Pool::ItemPool<Mesh>			m_meshPool;				// メッシュ
		Pool::ItemPool<AnimationData>	m_animationPool;		// アニメーション
		Pool::ItemPool<Texture>		m_texturePool;			// テクスチャ
		Pool::ItemPool<Shader>		m_shaderPool;			// シェーダー
		Pool::ItemPool<ShaderLibrary>	m_shaderLibraryPool;	// シェーダーライブラリ
		Pool::ItemPool<StateMachineAsset> m_stateMachinePool;	// ステートマシン

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

	// プールの取得
	template<> inline const Pool::ItemPool<Model>& ResourceManager::GetPool<Model>() const					{ return m_modelPool; }
	template<> inline const Pool::ItemPool<Material>& ResourceManager::GetPool<Material>() const				{ return m_materialPool; }
	template<> inline const Pool::ItemPool<Mesh>& ResourceManager::GetPool<Mesh>() const						{ return m_meshPool; }
	template<> inline const Pool::ItemPool<AnimationData>& ResourceManager::GetPool<AnimationData>() const	{ return m_animationPool; }
	template<> inline const Pool::ItemPool<Texture>& ResourceManager::GetPool<Texture>() const				{ return m_texturePool; }
	template<> inline const Pool::ItemPool<Shader>& ResourceManager::GetPool<Shader>() const					{ return m_shaderPool; }
	template<> inline const Pool::ItemPool<ShaderLibrary>& ResourceManager::GetPool<ShaderLibrary>() const	{ return m_shaderLibraryPool; }
	template<> inline const Pool::ItemPool<StateMachineAsset>& ResourceManager::GetPool<StateMachineAsset>() const	{ return m_stateMachinePool; }
	

	// テンプレート明示特殊化
	template<> inline Pool::ItemPool<Model>& ResourceManager::RefPool<Model>()					{ return m_modelPool; }
	template<> inline Pool::ItemPool<Material>& ResourceManager::RefPool<Material>()				{ return m_materialPool; }
	template<> inline Pool::ItemPool<Mesh>& ResourceManager::RefPool<Mesh>()						{ return m_meshPool; }
	template<> inline Pool::ItemPool<AnimationData>& ResourceManager::RefPool<AnimationData>()	{ return m_animationPool; }
	template<> inline Pool::ItemPool<Texture>& ResourceManager::RefPool<Texture>()				{ return m_texturePool; }
	template<> inline Pool::ItemPool<Shader>& ResourceManager::RefPool<Shader>()					{ return m_shaderPool; }
	template<> inline Pool::ItemPool<ShaderLibrary>& ResourceManager::RefPool<ShaderLibrary>()	{ return m_shaderLibraryPool; }
	template<> inline Pool::ItemPool<StateMachineAsset>& ResourceManager::RefPool<StateMachineAsset>()	{ return m_stateMachinePool; }
}