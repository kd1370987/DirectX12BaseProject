#include "RenderingOption.h"

#include "../../Editor/Helper/EditorHelper.inl"

void Engine::Option::GraphicsOptions::RenderingOption::DrawEdit()
{
	ImGui::Checkbox("isZPre", &isZPre);
	ImGui::Checkbox("useJitter (TAA)", &useJitter);
}

void Engine::Option::GraphicsOptions::RenderingOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("isZPre", isZPre);
	a_archive.Field("useJitter", useJitter);
}
