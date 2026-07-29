#include "RenderingOption.h"

#include "../../Editor/EditorUI/EditorUI.h"

void Engine::Option::GraphicsOptions::RenderingOption::DrawEdit()
{
	ImGui::Checkbox("isZPre", &isZPre);
	ImGui::Checkbox("useJitter (TAA)", &useJitter);

	// シェーディングモデル
	Handle<Resource::ShadingModelTable> _temp;
	Editor::UI::DrawAssetSelectCombo<Resource::ShadingModelTable>(
		"Change Shading Model",
		"ShadingModelTable",
		defaultShadingModelTable,
		_temp
	);
}

void Engine::Option::GraphicsOptions::RenderingOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("isZPre", isZPre);
	a_archive.Field("useJitter", useJitter);
	a_archive.Field("defaultShadingModelTable", defaultShadingModelTable);
}
