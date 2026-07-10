#include "InspectorPanel.h"

#include "AssetInspector/AssetInspector.h"

#include "EntityInspector/EntityInspector.h"

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
	default:
		break;
	}
}
