#pragma once

struct EntityLocation;

class ComponentEdit;

class ECSView
{
public:

	void Init();

	void Draw();

	std::shared_ptr<ComponentEdit> GetCompEdit();

private:

	// ヒエラルキー
	void HierarchyWindow();

	void DrawEntity(const EntityLocation& a_location);

	// インスペクターウィンドウ
	void InspectorWindow();

private:

	// 現在選択中のエンティティ
	ECS::Entity m_currentEntity = ECS::Limits::INVALID_ENTITY;

	// コンポーネントごとのエディット設定
	std::shared_ptr<ComponentEdit> m_spCompEdlit = nullptr;
};