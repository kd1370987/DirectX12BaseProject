#pragma once

namespace Engine
{
	namespace ECS
	{
		class World;
	}
}

namespace Engine::Resource
{
	class Prefab
	{
	public:
		Prefab();
		~Prefab() = default;
		NON_COPYABLE_MOVABLE(Prefab);

		// データの追加
		void AddComponent(ECS::ComponentTypeID a_compTypeID,uint8_t* a_pData);

		// データの削除
		void RemoveComponent(ECS::ComponentTypeID a_compTypeID);

		// シリアライズ処理
		void Archive(Persistence::Archive& a_ar,ECS::World* a_pWorld);

	private:

		// 生成エンティティシグネチャ
		ECS::Signature m_sigunature;

		// データ
		std::unordered_map<ECS::ComponentTypeID, uint8_t> m_dataMap = {};

	};
}