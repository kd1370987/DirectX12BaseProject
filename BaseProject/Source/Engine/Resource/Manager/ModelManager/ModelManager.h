#pragma once

namespace Engine::Resource
{
	//==========================================================================================
	// 
	// モデル管理クラス
	// 
	//==========================================================================================
	class ModelManager
	{
	public:

		//------------------------------------------------------------------------------------------
		// リソースの読み込み
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Model> LoadModel(const std::string& a_path);

		//------------------------------------------------------------------------------------------
		// リソースの取得
		//------------------------------------------------------------------------------------------
		const Engine::Resource::Model* GetModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);
		Engine::Resource::Model* RefModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);

		std::vector<SharedSlot<Model>>& GetAllModel();

	private:

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Model> Add(const Model& a_model);		// 追加
		void Subtract(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);	// 削除

	private:

		// 読み込まれたモデルデータを管理
		std::vector<SharedSlot<Model>> m_slotStorage = {};

		// 使用可能場所リスト
		std::queue<Index> m_indexQueue = {};
		UINT m_indexQueueMaxSize = 0;

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