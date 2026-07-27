#include "InspectorPanel.h"

#include "AssetInspector/AssetInspector.h"

#include "EntityInspector/EntityInspector.h"

#include "../../../GameObject/BaseObject/BaseObject.h"

void Engine::Editor::InspectorPanel::OnDrawImGui(EditorContext& a_editContext)
{
	switch (a_editContext.eInspectorType)
	{
	case EInspectorType::None :
		ImGui::Text("No selected");
		break;
	case EInspectorType::Entity:
		Inspector::EntityInspector(a_editContext);
		break;
	case EInspectorType::Asset:
		Inspector::AssetInspector(a_editContext);
		break;
	case EInspectorType::Game:
		// GameObjectManager管理下のオブジェクトのエディターを描画
		if (a_editContext.pGameObject)
		{
			ImGui::Text("%s", a_editContext.pGameObject->GetEditorName());
			ImGui::Separator();
			a_editContext.pGameObject->DrawInspector();
		}
		else
		{
			ImGui::Text("No selected object");
		}
		break;
	default:
		break;
	}
}
