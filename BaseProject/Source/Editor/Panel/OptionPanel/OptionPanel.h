#pragma once
#include "../IPanel.h"
#include "Editor/EditorCommon.h"

namespace Editor
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