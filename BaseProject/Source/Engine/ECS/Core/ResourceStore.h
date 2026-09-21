#pragma once

#include "../ResourceTypeManager/ResourceTypeManager.h"
#include "../ResourceWrapper/ResourceWrapper.h"

namespace Engine::ECS
{
	//==========================================================================================
	// ワールド寿命のリソース置き場
	//
	// 型ごとに1つだけ持つ(シングルトンリソース)。登録は初期化時のみで、
	// 実行中は引くだけ。アプリ寿命のものは EngineServices に置くこと。
	//==========================================================================================
	class ResourceStore
	{
	public:

		// リソースの登録 : すでにあれば何もしない
		template<typename ResourceType, typename... Args>
		void Add(Args&&... a_args);

		// リソースの参照 : 無ければ止める
		template<typename ResourceType>
		ResourceType& Get();

		// リソース生存チェック
		template<typename ResourceType>
		bool Has() const;

	private:

		// インターフェースポインタでリソースを保存
		std::unordered_map<ResourceTypeID, std::unique_ptr<IResourceWrapper>> m_resourceMap = {};
	};

	template<typename ResourceType, typename ...Args>
	inline void ResourceStore::Add(Args && ...a_args)
	{
		// ID取得
		ResourceTypeID _id = ResourceTypeManager::GetID<ResourceType>();

		if (m_resourceMap.find(_id) == m_resourceMap.end())
		{
			// unique_ptrを使って安全にアップキャストして保持
			// ランタイム中では行わずに初期登録時のみ走る
			m_resourceMap.emplace(_id, std::make_unique<ResourceWrapper<ResourceType>>(std::forward<Args>(a_args)...));
		}
	}

	template<typename ResourceType>
	inline ResourceType& ResourceStore::Get()
	{
		// IDを検索
		ResourceTypeID _id = ResourceTypeManager::GetID<ResourceType>();
		auto _it = m_resourceMap.find(_id);

		// 見つからなければ止める。
		// 参照で返すので返せるものが無く、ログだけ出して進むと end() を参照外しする
		if (_it == m_resourceMap.end())
		{
			ENGINE_ERROR("ECS::World : Resource not found (%s)", typeid(ResourceType).name());
			assert(0 && "ECS::World : 登録されていないリソースです");
			std::abort();
		}

		// RTTIによる型チェックを行わずに型が一致している前提でキャスト
		auto* _wrapper = static_cast<ResourceWrapper<ResourceType>*>(_it->second.get());
		return _wrapper->data;
	}

	template<typename ResourceType>
	inline bool ResourceStore::Has() const
	{
		// IDを検索してマップ内に存在するかどうかを返す
		ResourceTypeID _id = ResourceTypeManager::GetID<ResourceType>();
		return m_resourceMap.find(_id) != m_resourceMap.end();
	}
}
