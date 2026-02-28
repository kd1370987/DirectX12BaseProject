#pragma once

struct EntityLocation;

class ComponentEdit;

class World;

class ECSView
{
public:

	void Init();

	void Draw();

	std::shared_ptr<ComponentEdit> GetCompEdit();

private:

	// ヒエラルキー
	void HierarchyWindow(World* a_pWorld);

	void DrawEntity(World* a_pWorld,const EntityLocation& a_location);

	// インスペクターウィンドウ
	void InspectorWindow(World* a_pWorld);

private:

	// 現在選択中のエンティティ
	ECS::Entity m_currentEntity = ECS::Limits::INVALID_ENTITY;

	// コンポーネントごとのエディット設定
	std::shared_ptr<ComponentEdit> m_spCompEdlit = nullptr;
};