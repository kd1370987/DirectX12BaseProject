#include "InputOption.h"

#include "../../Input/InputManager/InputManager.h"

void Engine::Option::ProjectOptions::InputOption::DrawEdit()
{
	if (ImGui::Checkbox("isCursorLockedToCenter", &isCursorLockedToCenter))
	{
		Input::InputManager::Instance().SetCursorCentered(isCursorLockedToCenter);
	}
}

void Engine::Option::ProjectOptions::InputOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("isCursorLockedToCenter", isCursorLockedToCenter);
}

