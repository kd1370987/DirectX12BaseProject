#include "SceneManagerEditor.h"

#include "../../../Application/App.h"
#include "../../../Application/Scene/SceneManager.h"

#include "../../MainEngine.h"

namespace Engine::Editor
{
	void SceneManagerEditor::Init()
	{
	}

	void SceneManagerEditor::Draw(UINT a_widht, UINT a_height)
	{
		// --- ショートカット判定 ---
		// Ctrl+S で保存 (名前付き保存 or 上書き保存)
		// ※InputTextなどでキー入力中ではない時、もしくはCtrl+Sは特別に許可する
		m_isSaveShortcut = ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S);
		//m_canOverwrite = !m_sceneGuid.empty();
		m_doOverwrite = false;
		if (m_isSaveShortcut)
		{
			if (m_canOverwrite)
			{
				// シーンの上書き
				m_doOverwrite = true;
			}
			else
			{
				// シーンに名前を付けて保存
				m_openSaveAsPopup = true;
				m_sceneNameInput = "";
			}
		}

		// シーンの走査
		if (ImGui::Begin("Scene"))
		{
			// シーンのファイル
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File"))
				{
					// シーンの新規作成
					if (ImGui::MenuItem("Create new scene"))
					{
						ENGINE_LOG("新規シーンを作成しました。");
					}

					ImGui::Separator();

					// ---- Loadメニュー ----
					// シーンの読み込み
					if (ImGui::MenuItem("Load scene..."))
					{
						m_openLoadPopup = true;
					}

					ImGui::Separator();

					// ---- Saveメニュー ----
					// 上書き保存
					if (ImGui::MenuItem("Save scene", "Ctrl+S", false, m_canOverwrite))
					{
						m_doOverwrite = true;
					}

					// 名前を付けて保存
					if (ImGui::MenuItem("Save scene with Name..."))
					{
						m_openSaveAsPopup = true;
						m_sceneNameInput = "";
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			// 現在のシーンを取得
			auto* _pScene = SceneManager::Instance().GetCurrentTopScene();
			if (!_pScene)
			{
				// シーンが見つからなかったとき
				ImGui::Text("Not found current scnen");
			}
			else
			{

			}
		}
		ImGui::End();
	}
	void SceneManagerEditor::LoadScenePopup()
	{}
	void SceneManagerEditor::SaveScenePopup()
	{}
}