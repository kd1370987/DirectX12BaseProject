#pragma once

#include "../IPanel.h"

namespace Engine::Editor
{
	class RenderGraphResourceViewPanel : public IPanel
	{
	public:
		~RenderGraphResourceViewPanel() override = default;

		const char* GetName() const override { return "RenderGraphResourceView"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	private:
		float m_windowWidth = 1920;
		float m_windowHeight = 1080;
	};
}