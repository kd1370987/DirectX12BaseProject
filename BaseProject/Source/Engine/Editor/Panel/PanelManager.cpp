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

		for (auto& _panel : m_upPanelVec)
		{
			if (ImGui::Begin(_panel->GetName(),&_panel->m_isOpen,_panel->GetFlags()))
			{
				_panel->OnDrawImGui(m_editContext);
			}
			ImGui::End();
		}
	}
}
