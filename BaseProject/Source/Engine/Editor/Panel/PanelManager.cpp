#include "PanelManager.h"

#include "../Panel/RenderGraphResourceViewPanel/RenderGraphResourceViewPanel.h"
#include "../Panel/AssetDataBasePanel/AssetDataBasePanel.h"
#include "../Panel/InspectorPanel/InspectorPanel.h"
#include "../Panel/HierarchyPanel/HierarchyPanel.h"
#include "../Panel/GameObjectHierarchyPanel/GameObjectHierarchyPanel.h"
#include "../Panel/SceneViewPanel/SceneViewPanel.h"
#include "../Panel/OptionPanel/OptionPanel.h"
#include "../Panel/ProfilerPanel/ProfilerPanel.h"
#include "../Panel/LogPanel/LogPanel.h"

#include "../../Scene/SceneManager/SceneManager.h"
#include "../../ECS/World/World.h"
#include "../../GameObject/GameObjectManager/GameObjectManager.h"

namespace  Engine::Editor
{
	void PanelManager::Init(EditorCamera* a_pEditorCamera, Profiler* a_pProfiler)
	{
		RegisterPanel<RenderGraphResourceViewPanel>();
		RegisterPanel<AssetDataBasePanel>();
		RegisterPanel<InspectorPanel>();
		RegisterPanel<HierarchyPanel>();
		RegisterPanel<GameObjectHierarchyPanel>();
		RegisterPanel<SceneViewPanel>();
		RegisterPanel<OptionPanel>();
		RegisterPanel<ProfilerPanel>();
		RegisterPanel<LogPanel>();

		m_editContext.pEditorCamera = a_pEditorCamera;
		m_editContext.pProfiler = a_pProfiler;
	}

	void PanelManager::OnDrawPanels()
	{
		// 複数選択の修飾キー状態をフレーム頭で確定させる。
		// 各パネルが個別にキーを見ると、パネルごとに判定タイミングがずれるので
		// ここで一度だけ取って全パネルで共有する。
		m_editContext.m_isSelecting = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

		// 選択中のものが前のシーンの遺物になっていないか確かめる
		ValidateContext();

		for (auto& _panel : m_upPanelVec)
		{
			if (ImGui::Begin(_panel->GetName(),&_panel->m_isOpen,_panel->GetFlags()))
			{
				_panel->OnDrawImGui(m_editContext);
			}
			ImGui::End();
		}
	}

	void PanelManager::ClearSceneContext()
	{
		m_editContext.ClearSceneContext();
	}

	void PanelManager::ValidateContext()
	{
		//------------------------------------------------------------------
		// ECSエンティティ
		//------------------------------------------------------------------
		Engine::ECS::World* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit())
		{
			m_editContext.ClearEntitySelection();
		}
		else
		{
			// 死んだものだけを取り除く。
			// 全消しにすると、複数選択中に1体だけ消えたときまで選択が飛ぶ
			std::erase_if(
				m_editContext.selectedEntities,
				[_pWorld](const ECS::Entity& a_entity)
				{
					if (a_entity == ECS::Limits::INVALID_ENTITY) return false;	// 未選択のダミーは残す
					return !_pWorld->IsAliveEntity(a_entity);
				}
			);

			// 空にはしない(未選択のときは INVALID を1つだけ持つ)という不変条件を守る
			if (m_editContext.selectedEntities.empty())
			{
				m_editContext.ClearEntitySelection();
			}
		}

		//------------------------------------------------------------------
		// ECS外オブジェクト
		//------------------------------------------------------------------
		if (!m_editContext.pGameObject) return;

		auto* _pObjManager = Engine::Scene::SceneManager::Instance().RefGameObjectManager();
		if (!_pObjManager || !_pObjManager->IsManaged(m_editContext.pGameObject))
		{
			m_editContext.pGameObject = nullptr;

			// インスペクターが Game のまま「選択なし」で残らないようにする
			if (m_editContext.eInspectorType == EInspectorType::Game)
			{
				m_editContext.eInspectorType = EInspectorType::None;
			}
		}
	}
}
