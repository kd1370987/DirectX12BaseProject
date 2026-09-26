#include "RenderingOption.h"

#include "../../Editor/Helper/EditorField.inl"

void Engine::Option::GraphicsOptions::RenderingOption::DrawEdit(const ECS::EngineServices&)
{
	Engine::Editor::Field("isZPre", isZPre);
	Engine::Editor::Field("useJitter (TAA)", useJitter);
}

void Engine::Option::GraphicsOptions::RenderingOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("isZPre", isZPre);
	a_archive.Field("useJitter", useJitter);
}
