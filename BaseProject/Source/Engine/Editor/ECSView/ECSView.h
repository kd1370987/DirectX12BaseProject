#pragma once


namespace Engine::ECS
{
	struct EntityLocation;

	class World;
}
namespace Engine::Editor
{
	// ECSを管理するためのエディター
	class ECSView
	{
	public:

		void Init();

		void Draw(UINT a_widht, UINT a_height);

	private:

		// ヒエラルキー
		void HierarchyWindow(Engine::ECS::World* a_pWorld);
		void EntityFilter();

		void DrawEntityList(Engine::ECS::World* a_pWorld);

		void DrawEntity(Engine::ECS::World* a_pWorld, const Engine::ECS::EntityLocation& a_location);

		// エンティティの追加
		void AddEntity(Engine::ECS::World* a_pWorld);

		// インスペクターウィンドウ
		void InspectorWindow(Engine::ECS::World* a_pWorld);
		void AddComponent(Engine::ECS::World* a_pWorld);
		void SubmitCommponent(Engine::ECS::World* a_pWorld,ECS::ComponentTypeID a_typeID);
		

	private:

		// 現在選択中のエンティティ
		Engine::ECS::Entity m_currentEntity = Engine::ECS::Limits::INVALID_ENTITY;

		// 検索時のフィルター
		enum class EFilterType
		{
			None,
			Player,
			Camera,
			Ground,
			UI,
		};
		EFilterType m_filterType = EFilterType::None;
	};
}