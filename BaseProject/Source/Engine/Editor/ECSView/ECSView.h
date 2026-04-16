#pragma once


namespace Engine::ECS
{
	struct EntityLocation;

	class World;
}
namespace Engine::Editor
{
	// 前方宣言
	class ComponentEdit;

	// ECSを管理するためのエディター
	class ECSView
	{
	public:

		void Init();

		void Draw(UINT a_widht, UINT a_height);

		std::shared_ptr<ComponentEdit> GetCompEdit();

	private:

		// ヒエラルキー
		void HierarchyWindow(Engine::ECS::World* a_pWorld);
		void EntityFilter();

		void DrawEntity(Engine::ECS::World* a_pWorld, const Engine::ECS::EntityLocation& a_location);

		// インスペクターウィンドウ
		void InspectorWindow(Engine::ECS::World* a_pWorld);

	private:

		// 現在選択中のエンティティ
		Engine::ECS::Entity m_currentEntity = Engine::ECS::Limits::INVALID_ENTITY;

		// コンポーネントごとのエディット設定
		std::shared_ptr<ComponentEdit> m_spCompEdlit = nullptr;

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