#pragma once
#include "../IPanel.h"

namespace Engine
{
	namespace ECS
	{
		class World;
	}
}


namespace Engine::Editor
{
	class SceneViewPanel : public IPanel
	{
	public:
		~SceneViewPanel() override = default;

		const char* GetName() const override { return "SceneViewPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	private:
		void GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld);
	};
}