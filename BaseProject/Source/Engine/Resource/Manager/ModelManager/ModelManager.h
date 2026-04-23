#pragma once

namespace Engine::Resource
{
	class AssetManager;

	//==========================================================================================
	// 
	// モデル管理クラス
	// 
	//==========================================================================================
	class ModelManager
	{
	public:

		void Init(AssetManager* a_pAssetManager);

		//------------------------------------------------------------------------------------------
		// リソースの読み込み
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Model> LoadModel(const std::string& a_path);

		Handle<Model> Load(const Engine::GUID& a_guid);
		Handle<Model> Request(const std::string& a_path);

		//------------------------------------------------------------------------------------------
		// リソースの取得
		//------------------------------------------------------------------------------------------
		const Engine::Resource::Model* GetModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);
		Engine::Resource::Model* RefModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);
		const Engine::Resource::Handle<Engine::Resource::Model>& GetHandle(const std::string& a_name);

		std::vector<SharedSlot<Model>>& GetAllModel();

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		bool Has(const Engine::GUID& a_guid) const;

	private:

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Model> Add(const Model& a_model);		// 追加
		void Subtract(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);	// 削除


	private:

		std::unordered_map<std::string, Engine::Resource::Handle<Engine::Resource::Model>> m_handleMap = {};

		// 読み込まれたモデルデータを管理
		std::vector<SharedSlot<Model>> m_slotStorage = {};


		// 使用可能場所リスト
		std::queue<Index> m_indexQueue = {};
		UINT m_indexQueueMaxSize = 0;

		
		// アセットマネージャーに依存
		AssetManager* m_pAssetManager = nullptr;

		// GUIDとハンドルを結ぶ
		std::unordered_map<Engine::GUID, Handle<Model>> m_guidToModelHandleMap = {};

		// ハンドル管理
		Storage::HandleStorage<Model> m_handleStorage = {};

		// 実データ
		std::vector<Model> m_modelVec = {};


	private:

		ModelManager();
		~ModelManager();

	public:

		static ModelManager& Instnace()
		{
			static ModelManager _instance;
			return _instance;
		}
	};
}