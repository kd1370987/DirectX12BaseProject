#pragma once

namespace Engine::Resource
{
	class AssetDatabase;

	//==========================================================================================
	// 
	// モデル管理クラス
	// 
	//==========================================================================================
	class ModelManager
	{
	public:

		void Init(AssetDatabase* a_pAssetDatabase);

		//------------------------------------------------------------------------------------------
		// リソースの読み込み
		//------------------------------------------------------------------------------------------
		Handle<Model> Load(const Engine::GUID& a_guid);
		Handle<Model> Request(const std::string& a_path);

		//------------------------------------------------------------------------------------------
		// リソースの取得
		//------------------------------------------------------------------------------------------
		const Model* GetModel(const Handle<Model>& a_handle) const;
		Model* RefModel(const Handle<Model>& a_handle);

		// ハンドルの取得
		const Handle<Model>& GetHandle(const Engine::GUID& a_guid);

		// 全モデル取得
		const std::unordered_map<Engine::GUID, Handle<Model>> GetAllModelHandleMap();
		const std::vector<Model>& GetAllModel() const;

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		bool Has(const Engine::GUID& a_guid) const;
		
	private:
		
		// アセットマネージャーに依存
		AssetDatabase* m_pAssetDatabase = nullptr;

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