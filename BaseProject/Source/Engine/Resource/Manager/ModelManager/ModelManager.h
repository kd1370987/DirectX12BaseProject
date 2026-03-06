#pragma once

namespace Engine::Resource
{

	template<typename Data>
	struct SharedSlot
	{
		Data data;
		Generation gen = Limits::INVALID_GENERATION;
		uint32_t sharedCount = 0;
	};


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
		Engine::Resource::Handle<Engine::Resource::Model> LoadModel(std::string_view a_metaName);

		//------------------------------------------------------------------------------------------
		// リソースの取得
		//------------------------------------------------------------------------------------------
		const Engine::Resource::Model* GetModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle);

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
	};
}