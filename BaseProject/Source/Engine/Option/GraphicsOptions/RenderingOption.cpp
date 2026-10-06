#include "RenderingOption.h"

#include "Engine/EditorField/EditorField.inl"

void Engine::Option::GraphicsOptions::RenderingOption::DrawEdit(const ECS::EngineServices&)
{
	Engine::EditorField::Field("isZPre", isZPre);
	Engine::EditorField::Field("useJitter (TAA)", useJitter);
}

void Engine::Option::GraphicsOptions::RenderingOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("isZPre", isZPre);
	a_archive.Field("useJitter", useJitter);
}
