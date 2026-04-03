#pragma once


class ComponentEdit;
namespace Engine::ECS
{
	struct EntityLocation;

	class World;
}

class ECSView
{
public:

	void Init();

	void Draw();

	std::shared_ptr<ComponentEdit> GetCompEdit();

private:

	// ヒエラルキー
	void HierarchyWindow(Engine::ECS::World* a_pWorld);

	void DrawEntity(Engine::ECS::World* a_pWorld,const Engine::ECS::EntityLocation& a_location);

	// インスペクターウィンドウ
	void InspectorWindow(Engine::ECS::World* a_pWorld);

private:

	// 現在選択中のエンティティ
	Engine::ECS::Entity m_currentEntity = Engine::ECS::Limits::INVALID_ENTITY;

	// コンポーネントごとのエディット設定
	std::shared_ptr<ComponentEdit> m_spCompEdlit = nullptr;
};