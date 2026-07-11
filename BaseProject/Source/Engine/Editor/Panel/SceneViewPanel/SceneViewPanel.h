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
		ImGuiWindowFlags GetFlags() const override { return ImGuiWindowFlags_MenuBar; }
	private:

		// ギズモ
		void GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect, const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld);

		// メニューバー
		void SceneFileMenu(EditorContext& a_editContext);

		// シーンファイル操作
		void LoadScenePopup();
		void SaveScenePopup();

		void OpenSavePopup();
		void SaveScene(const Engine::GUID& a_guid);

	private:

		// シーンファイル操作系
		bool m_isLoadScenePopup = false;
		bool m_openLoadPopup = false;
		bool m_openSaveAsPopup = false;
		std::string m_sceneNameInput = "";

		// 操作用変数
		bool m_isSaveShortcut = false;
		bool m_canOverwrite = false;
		bool m_doOverwrite = false;

		Engine::GUID m_currentSceneGUID = Engine::DefaultGUID;
	};
}