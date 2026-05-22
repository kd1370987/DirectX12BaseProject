#include "SystemView.h"

#include "Application/Scene/SceneManager.h"
#include "Engine/ECS/World/World.h"

namespace Engine::Editor
{
	void Engine::Editor::SystemView::Init()
	{}

	void Engine::Editor::SystemView::Draw(UINT a_widht, UINT a_height)
	{
		// ワールド取得
		Engine::ECS::World* _pWorld = SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit()) return;

		if (ImGui::Begin("SystemsViewer", nullptr, ImGuiWindowFlags_MenuBar))
		{
			// システム配列取得
			

		}
	}
}