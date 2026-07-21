#pragma once
#include "../IPanel.h"

namespace Engine
{
	namespace ECS
	{
		class World;
	}
}

namespace Engine::Editor
{
	class HierarchyPanel : public IPanel
	{
	public:
		~HierarchyPanel() override = default;

		const char* GetName() const override { return "HierarchyPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	private:

		// 空のエンティティの追加
		void AddEntity(ECS::World* a_pWorld);

		// プレハブから実体を生成して追加
		void InstantiatePrefab(ECS::World* a_pWorld, const Engine::GUID& a_guid);

		// エンティティノードの描画
		void DrawEntityNode(EditorContext& a_editContext,Engine::ECS::World* a_pWorld, const Engine::ECS::Entity& a_entity);

		// ドラッグアンドドロップの制御
		void HandleDragAndDrop(ECS::World* a_pWorld, const ECS::Entity& a_entity, const std::string& a_label);
		// 親子関係の再構築処理
		void AttachChild(ECS::World* a_pWorld, const ECS::Entity& a_parent, const ECS::Entity& a_child);
		void DettachChild(ECS::World* a_pWorld, const ECS::Entity& a_child);
		// 子供の取得
		std::vector<ECS::Entity> GetChildEntities(Engine::ECS::World* a_pWorld, ECS::Entity a_parent);

	private:
		// 検索時のフィルター
		enum class EFilterType
		{
			None,
			Player,
			Camera,
			Ground,
			UI,
		};
		EFilterType m_filterType;
	};
}