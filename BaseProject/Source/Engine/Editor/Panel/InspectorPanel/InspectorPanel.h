#pragma once

#include "../IPanel.h"

namespace Engine::Editor
{
	class InspectorPanel : public IPanel
	{
	public:
		~InspectorPanel() override = default;

		const char* GetName() const override { return "InspectorPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	private:
	};
}