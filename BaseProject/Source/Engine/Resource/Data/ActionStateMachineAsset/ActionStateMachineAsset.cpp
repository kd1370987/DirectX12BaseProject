#include "ActionStateMachineAsset.h"

#include "../../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	//======================================================================================
	// ノードのArchive
	//======================================================================================
	void ActionNode::Archive(Persistence::Archive& a_arch)
	{
		// 共通の「つなぎ情報」(名前・座標・各種ID)
		ArchiveTopology(a_arch);

		// ゲームプレイ固有: 行動制約
		a_arch.Field("canMove", canMove);
		a_arch.Field("canRotate", canRotate);
		a_arch.Field("invincible", invincible);
		a_arch.Field("moveSpeedScale", moveSpeedScale);
	}

	//======================================================================================
	// 保存 / 読み込み
	//======================================================================================
	void ActionStateMachineAsset::Save(const std::string& a_savePath)
	{
		m_editor.SyncPositions(m_graph);

		auto _dir = FileUtility::GetDirFromPath(a_savePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_savePath);
		Persistence::Archive _arch(Persistence::Archive::Mode::Save, _dir, _fileName, "asm");

		_arch.Field("m_name", m_name);
		m_graph.SaveGraph(_arch);
	}

	void ActionStateMachineAsset::Load(const std::string& a_fileDir, const std::string& a_fileName)
	{
		LoadInternal(a_fileDir, a_fileName);
	}

	void ActionStateMachineAsset::Load(const std::string& a_filePath)
	{
		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		LoadInternal(_dir, _fileName);
	}

	void ActionStateMachineAsset::LoadInternal(const std::string& a_fileDir, const std::string& a_fileName)
	{
		Release();

		Persistence::Archive _arch(
			Persistence::Archive::Mode::Load, a_fileDir, a_fileName, "asm",
			Persistence::Archive::ArchiveFormat::Json);

		_arch.Field("m_name", m_name);
		m_graph.LoadGraph(_arch);

		m_editor.RequestApplyLoadedPositions();
	}

	void ActionStateMachineAsset::Release()
	{
		m_graph.Clear();
		m_editor.DestroyContext();
		m_name.clear();
	}

	//======================================================================================
	// エディター
	//======================================================================================
	void ActionStateMachineAsset::EditImGui(const Handle<ActionStateMachineAsset>& a_handle)
	{
		if (ImGui::Button("Save"))
		{
			auto _guid = ResourceManager::Instance().GetCache<ActionStateMachineAsset>(a_handle);
			auto _path = AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			Save(_path);
			Editor::MainEditor::Instance().AddLog("%s", _path.c_str());
			Editor::MainEditor::Instance().AddLog(" : Save ActionStateMachineAsset\n");
		}
		ImGui::Separator();

		// ノード本体(行動制約UI)だけを注入して汎用ノードエディタを描画
		m_editor.Draw(m_graph,
			[](ActionNode& a_node)
			{
				ImGui::PushItemWidth(120.0f);
				ImGui::Checkbox("CanMove", &a_node.canMove);
				ImGui::Checkbox("CanRotate", &a_node.canRotate);
				ImGui::Checkbox("Invincible", &a_node.invincible);
				ImGui::DragFloat("SpeedScale", &a_node.moveSpeedScale, 0.01f, 0.0f, 10.0f);
				ImGui::PopItemWidth();
			});
	}
}
