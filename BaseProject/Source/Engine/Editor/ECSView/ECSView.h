#pragma once

#include "SystemView/SystemView.h"

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

		ECS::Entity GetSelectEntity() { return  m_currentEntity; }

	private:

		// ---- ヒエラルキー ----
		void HierarchyWindow(Engine::ECS::World* a_pWorld);
		void EntityFilter();

		void DrawEntityList(Engine::ECS::World* a_pWorld);
		void DrawEntity(Engine::ECS::World* a_pWorld, const Engine::ECS::Entity& a_entity);
		// ドラッグアンドドロップの制御
		void HandleDragAndDrop(ECS::World* a_pWorld, const ECS::Entity& a_entity,const std::string& a_label);
		// 親子関係の再構築処理
		void AttachChild(ECS::World* a_pWorld, const ECS::Entity& a_parent,const ECS::Entity& a_child);
		void DettachChild(ECS::World* a_pWorld,const ECS::Entity& a_child);
		// 子供の取得
		std::vector<ECS::Entity> GetChildEntities(Engine::ECS::World* a_pWorld, ECS::Entity a_parent);

		// エンティティの追加
		void AddEntity(Engine::ECS::World* a_pWorld);

		// ---- インスペクターウィンドウ ----
		void InspectorWindow(Engine::ECS::World* a_pWorld);
		void AddComponent(Engine::ECS::World* a_pWorld);
		void SubmitCommponent(Engine::ECS::World* a_pWorld,ECS::ComponentTypeID a_typeID);
		
		// ----- ヘルパー -----
		ECS::Entity FindEntityByGUID(Engine::ECS::World* a_pWorld, const Engine::GUID& a_guid);


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

		SystemView m_systemView;
	};
}