#include "GameFlowStateMachine.h"

#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

namespace App::Game
{
	//======================================================================================
	// 保存 / 読み込み
	//======================================================================================
	void GameFlowStateMachine::Save(const std::string& a_savePath)
	{
		// ImNodes上の現在座標をノードへ書き戻してから保存
		m_editor.SyncPositions(m_graph);

		auto _dir = FileUtility::GetDirFromPath(a_savePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_savePath);
		Engine::Persistence::Archive _arch(Engine::Persistence::Archive::Mode::Save, _dir, _fileName, "stet");

		// GameFlow固有ヘッダ
		_arch.Field("m_farstSceneGUID", m_farstSceneGUID);

		// グラフ本体(ノード・矢印・パラメータ・既定開始)
		m_graph.SaveGraph(_arch);
	}

	void GameFlowStateMachine::Load(const std::string& a_filePath)
	{
		if (a_filePath.empty()) return;
		m_filePath = a_filePath;

		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);

		Release();

		Engine::Persistence::Archive _arch(
			Engine::Persistence::Archive::Mode::Load, _dir, _fileName, "stet",
			Engine::Persistence::Archive::ArchiveFormat::Json);

		_arch.Field("m_farstSceneGUID", m_farstSceneGUID);
		m_graph.LoadGraph(_arch);

		// 復元した座標をImNodesへ反映
		m_editor.OnLoaded(m_graph);
	}

	void GameFlowStateMachine::Release()
	{
		m_graph.Clear();
		m_editor.DestroyContext();

		m_farstSceneGUID = {};
		m_currentStateHash = 0;
		m_params = {};
	}

	//======================================================================================
	// エディター
	//======================================================================================
	void GameFlowStateMachine::EditImGui()
	{
		// 保存(ファイルパスはマシン固有なのでここで扱う)
		if (ImGui::Button("Save"))
		{
			Save(m_filePath);
		}
		ImGui::Separator();

		// 初期シーン設定(GameFlow固有UI)
		FarstSceneSetting();

		// ノード本体だけを注入して汎用ノードエディタを描画
		m_editor.Draw(m_graph,
			[](FlowNode& a_node)
			{
				ImGui::Text("SceneAsset");
				ImGui::Text("%s", a_node.sceneGUID.String().c_str());

				auto _fileName = Engine::Resource::AssetDatabase::Instance().GetFileNameFromGUID(a_node.sceneGUID);
				if (ImGui::BeginCombo(_fileName.c_str(), "Select..."))
				{
					for (auto& _prop : Engine::Resource::AssetDatabase::Instance().GetTypeMetaVec("Scene"))
					{
						bool _selected = (a_node.sceneGUID == _prop.guid);
						if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
						{
							a_node.sceneGUID = _prop.guid;
						}
					}
					ImGui::EndCombo();
				}
			});
	}

	//======================================================================================
	// ランタイム
	//======================================================================================
	Engine::GUID GameFlowStateMachine::Start()
	{
		// 初期シーンに一致するノードを既定開始に設定
		for (auto& [_hash, _node] : m_graph.Nodes())
		{
			if (_node.sceneGUID == m_farstSceneGUID)
			{
				m_graph.SetDefaultStartHash(_hash);
			}
		}
		m_currentStateHash = m_graph.GetDefaultStartHash();
		return m_farstSceneGUID;
	}

	void GameFlowStateMachine::SetTrigger(const std::string& a_triggerName)
	{
		m_params.SetBool(StringUtility::ToHash(a_triggerName), true);
	}

	void GameFlowStateMachine::SetBool(const std::string& a_name, bool a_value)
	{
		m_params.SetBool(StringUtility::ToHash(a_name), a_value);
	}

	void GameFlowStateMachine::SetInt(const std::string& a_name, int a_value)
	{
		m_params.SetInt(StringUtility::ToHash(a_name), a_value);
	}

	void GameFlowStateMachine::SetFloat(const std::string& a_name, float a_value)
	{
		m_params.SetFloat(StringUtility::ToHash(a_name), a_value);
	}

	bool GameFlowStateMachine::Evaluate(Engine::GUID& out_nextSceneGUID)
	{
		UINT _next = m_graph.Evaluate(m_currentStateHash, m_params);

		// 遷移が起きなければ現状維持
		if (_next == m_currentStateHash) return false;

		m_currentStateHash = _next;

		const FlowNode* _pNext = m_graph.GetStateNode(_next);
		out_nextSceneGUID = _pNext ? _pNext->sceneGUID : Engine::DefaultGUID;
		return true;
	}

	//======================================================================================
	// GameFlow固有UI
	//======================================================================================
	void GameFlowStateMachine::FarstSceneSetting()
	{
		ImGui::Text("Farst Scene : %s", m_farstSceneGUID.String().c_str());

		if (ImGui::Button("Farst Scene Setting"))
		{
			m_isFarstScenePopup = true;
		}

		if (m_isFarstScenePopup) { ImGui::OpenPopup("Farst Scene Asset"); m_isFarstScenePopup = false; }
		if (ImGui::BeginPopupModal("Farst Scene Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const auto& _sceneMetaVec = Engine::Resource::AssetDatabase::Instance().GetTypeMetaVec("Scene");
			if (_sceneMetaVec.empty())
			{
				ImGui::TextDisabled("Not find SceneAsset");
			}

			for (const auto& _sceneMeta : _sceneMetaVec)
			{
				if (ImGui::Selectable(_sceneMeta.fileName.c_str(), m_farstSceneGUID == _sceneMeta.guid))
				{
					m_farstSceneGUID = _sceneMeta.guid;
					ImGui::CloseCurrentPopup();

					// 既定開始ノードも更新
					for (auto& [_hash, _node] : m_graph.Nodes())
					{
						if (_node.sceneGUID == m_farstSceneGUID)
						{
							m_graph.SetDefaultStartHash(_hash);
						}
					}
				}
			}

			ImGui::Separator();
			if (ImGui::Button("Close", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}

		ImGui::Separator();
	}
}
