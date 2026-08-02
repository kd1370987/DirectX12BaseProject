#include "PanelManager.h"

#include "../Panel/RenderGraphResourceViewPanel/RenderGraphResourceViewPanel.h"
#include "../Panel/AssetDataBasePanel/AssetDataBasePanel.h"
#include "../Panel/InspectorPanel/InspectorPanel.h"
#include "../Panel/HierarchyPanel/HierarchyPanel.h"
#include "../Panel/GameObjectHierarchyPanel/GameObjectHierarchyPanel.h"
#include "../Panel/SceneViewPanel/SceneViewPanel.h"
#include "../Panel/OptionPanel/OptionPanel.h"
#include "../Panel/ProfilerPanel/ProfilerPanel.h"

namespace  Engine::Editor
{
	void PanelManager::Init(EditorCamera* a_pEditorCamera)
	{
		RegisterPanel<RenderGraphResourceViewPanel>();
		RegisterPanel<AssetDataBasePanel>();
		RegisterPanel<InspectorPanel>();
		RegisterPanel<HierarchyPanel>();
		RegisterPanel<GameObjectHierarchyPanel>();
		RegisterPanel<SceneViewPanel>();
		RegisterPanel<OptionPanel>();
		RegisterPanel<ProfilerPanel>();

		m_editContext.pEditorCamera = a_pEditorCamera;
	}

	void PanelManager::OnDrawPanels()
	{

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
