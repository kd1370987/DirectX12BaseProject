#pragma once

#include "../IPanel.h"

namespace Engine::Editor
{
	class AssetDataBasePanel : public IPanel
	{
	public:
		~AssetDataBasePanel() override = default;

		const char* GetName() const override { return "AssetDataBase"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	private:

		void AssetDataBaseExplorer(EditorContext& a_editContext);

		float m_windowWidth = 1920;
		float m_windowHeight = 1080;
	};
}