#include "SceneManagerEditor.h"

#include "../../../Application/App.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "../../Scene/BaseScene/BaseScene.h"

#include "../../MainEngine.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Editor
{
	void SceneManagerEditor::Init()
	{
	}

	void SceneManagerEditor::Draw()
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
		if (ImGui::Begin("Scene", 0,ImGuiWindowFlags_MenuBar))
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
		}

		LoadScenePopup();
		SaveScenePopup();

		ImGui::End();
	}

	void SceneManagerEditor::LoadScenePopup()
	{
		// --- ポップアップの処理 ---
		// シーン読み込みポップアップ
		if (m_openLoadPopup) { ImGui::OpenPopup("Load Scene Asset"); m_openLoadPopup = false; }
		if (ImGui::BeginPopupModal("Load Scene Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const auto& _sceneMetaVec = Resource::AssetDatabase::Instance().GetTypeMetaVec("Scene");
			if (_sceneMetaVec.empty())
			{
				ImGui::TextDisabled("Not find SceneAsset");
			}

			for (const auto& _sceneMeta : _sceneMetaVec)
			{
				if (ImGui::Selectable(_sceneMeta.fileName.c_str(), m_currentSceneGUID == _sceneMeta.guid))
				{

					// 現在のシーンを取得
					auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
					if (!_pScene)
					{
						ENGINE_LOG("シーンのセーブに失敗しました");
						return;
					}
					// 現在のシーンをセーブ
					SaveScene(m_currentSceneGUID);
					// 初期化
					_pScene->Enter();

					// シーンの読み込み
					LoadScene(_sceneMeta.guid);
					ENGINE_LOG("シーンを読み込みました : %s",_sceneMeta.fileName.c_str());

					// 前回のシーンを記憶する処理を追加予定
					
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::Separator();
			if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
	}

	void SceneManagerEditor::SaveScenePopup()
	{
		// 名前を付けて保存ポップアップ
		if (m_openSaveAsPopup) { ImGui::OpenPopup("Save Scene As"); m_openSaveAsPopup = false; }
		if (ImGui::BeginPopupModal("Save Scene As", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Input Filename (.scene) : ");
			ImGui::InputText("##scenename", &m_sceneNameInput);

			if (ImGui::Button("Save", ImVec2(120, 0)))
			{
				std::string _name = m_sceneNameInput;
				if (!_name.empty())
				{
					std::string _filepath = "Asset/Scenes/" + _name + _name;
					// フォルダがなければ作成
					std::filesystem::create_directories("Asset/Scenes/");

					// GUIDを発行
					Engine::GUID _guid = Resource::AssetDatabase::Instance().AddMetaData(_filepath, "Scene");
					m_currentSceneGUID = _guid;
					SaveScene(m_currentSceneGUID);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cansele", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
	}
	void SceneManagerEditor::LoadScene(const Engine::GUID& a_guid)
	{
		// 現在のシーンを取得
		auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
		if (!_pScene) 
		{
			ENGINE_LOG("シーンのロードに失敗しました");
			return;
		}

		// ファイルパスを取得
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_path.empty())
		{
			ENGINE_LOG("シーンのロードに失敗しました");
			return;
		}

		auto _fileDir = FileUtility::GetDirFromPath(_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(_path);
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "scene");
		_pScene->Archive(_ar);
	}
	void SceneManagerEditor::SaveScene(const Engine::GUID& a_guid)
	{
		// 現在のシーンを取得
		auto* _pScene = Engine::Scene::SceneManager::Instance().GetCurrentTopScene();
		if (!_pScene)
		{
			ENGINE_LOG("シーンのセーブに失敗しました");
			return;
		}

		// ファイルパスを取得
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_path.empty())
		{
			ENGINE_LOG("シーンのセーブに失敗しました");
			return;
		}

		auto _fileDir = FileUtility::GetDirFromPath(_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(_path);
		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "scene");
		_pScene->Archive(_ar);
	}
}