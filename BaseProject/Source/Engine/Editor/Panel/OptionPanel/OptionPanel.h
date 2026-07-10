#pragma once
#include "../IPanel.h"

namespace Engine::Editor
{
	class OptionPanel : public IPanel
	{
	public:
		~OptionPanel() override = default;

		const char* GetName() const override { return "OptionPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;

	private:
	};
}