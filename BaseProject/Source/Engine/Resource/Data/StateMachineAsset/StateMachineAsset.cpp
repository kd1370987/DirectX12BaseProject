#include "StateMachineAsset.h"

#include "../../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	//======================================================================================
	// ノードのArchive
	//======================================================================================
	void AnimStateNode::Archive(Persistence::Archive& a_arch)
	{
		// 共通の「つなぎ情報」(名前・座標・各種ID)
		ArchiveTopology(a_arch);

		// Animator固有: 再生アニメ情報
		a_arch.Field("animGUID", animGUID);
		a_arch.Field("speed", speed);
		a_arch.Field("isLoop", isLoop);
	}

	//======================================================================================
	// 保存
	//======================================================================================
	void StateMachineAsset::Save(const std::string& a_savePath)
	{
		// 保存直前: 各ノードの再生アニメ参照(playAnimData)からGUIDを取り出しておく
		auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
		if (_pModel)
		{
			for (auto& [_hash, _node] : m_graph.Nodes())
			{
				_node.animGUID = _pModel->GetAnimationGUIDFromHandle(_node.playAnimData);
			}
		}

		// ImNodes上の現在座標をノードへ書き戻す
		m_editor.SyncPositions(m_graph);

		auto _dir = FileUtility::GetDirFromPath(a_savePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_savePath);
		Persistence::Archive _arch(Persistence::Archive::Mode::Save, _dir, _fileName, "stet");

		// Animator固有ヘッダ
		_arch.Field("m_name", m_name);
		_arch.Field("m_modelGUID", m_modelGUID);

		// グラフ本体(ノード・矢印・パラメータ・既定開始)
		m_graph.SaveGraph(_arch);
	}

	//======================================================================================
	// 読み込み
	//======================================================================================
	void StateMachineAsset::Load(const std::string& a_fileDir, const std::string& a_fileName)
	{
		LoadInternal(a_fileDir, a_fileName);
	}

	void StateMachineAsset::Load(const std::string& a_filePath)
	{
		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		LoadInternal(_dir, _fileName);
	}

	void StateMachineAsset::LoadInternal(const std::string& a_fileDir, const std::string& a_fileName)
	{
		Release();

		Persistence::Archive _arch(
			Persistence::Archive::Mode::Load, a_fileDir, a_fileName, "stet",
			Persistence::Archive::ArchiveFormat::Json);

		// Animator固有ヘッダ
		_arch.Field("m_name", m_name);
		_arch.Field("m_modelGUID", m_modelGUID);
		m_modelHandle = ResourceManager::Instance().Load<Model>(m_modelGUID);

		// グラフ本体
		m_graph.LoadGraph(_arch);

		// モデルから各ノードの再生アニメ参照を復元(GUID → ハンドル)
		auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
		if (_pModel)
		{
			for (auto& [_hash, _node] : m_graph.Nodes())
			{
				_node.playAnimData = _pModel->GetAnimationHandleFromGUID(_node.animGUID);
			}
		}

		// 復元した座標は次のDrawでImNodesへ反映
		m_editor.RequestApplyLoadedPositions();
	}

	void StateMachineAsset::Release()
	{
		m_graph.Clear();
		m_editor.DestroyContext();
		m_name.clear();
		m_modelGUID = Engine::DefaultGUID;
		m_modelHandle = {};
	}

	//======================================================================================
	// エディター
	//======================================================================================
	void StateMachineAsset::EditImGui(const Handle<StateMachineAsset>& a_handle)
	{
		// 保存(ファイルパスはハンドル→GUID→パスで解決)
		if (ImGui::Button("Save"))
		{
			auto _guid = ResourceManager::Instance().GetCache<StateMachineAsset>(a_handle);
			auto _path = AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			Save(_path);
			Editor::MainEditor::Instance().AddLog("%s", _path.c_str());
			Editor::MainEditor::Instance().AddLog(" : Save StateMachinAsset\n");
		}
		ImGui::Separator();

		// アニメを付随させるための参照モデル選択(Animator固有)
		BindModelComb();
		ImGui::Separator();

		// ノード本体だけ(アニメ選択UI)を注入して汎用ノードエディタを描画
		m_editor.Draw(m_graph,
			[this](AnimStateNode& a_node)
			{
				if (m_modelHandle == Handle<Model>()) return;

				auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
				if (!_pModel) return;

				// 現在の再生アニメ名
				std::string _viewName = "Select...";
				if (a_node.playAnimData != Handle<AnimationData>())
				{
					auto* _pAnimData = ResourceManager::Instance().Get(a_node.playAnimData);
					if (_pAnimData) _viewName = _pAnimData->name;
				}

				ImGui::PushItemWidth(130.0f);

				// アニメ選択
				ImGui::Text("Animation");
				if (ImGui::BeginCombo("##ChangeAnimation", _viewName.c_str()))
				{
					for (auto& _handle : _pModel->GetAnimationHandles())
					{
						auto* _pAnimData = ResourceManager::Instance().Get(_handle);
						bool _selected = (a_node.playAnimData == _handle);
						if (ImGui::Selectable(_pAnimData->name.c_str(), _selected))
						{
							a_node.playAnimData = _handle;
						}
					}
					ImGui::EndCombo();
				}

				// 再生スピード
				ImGui::Text("Speed");
				ImGui::DragFloat("##AnimationSpeed", &a_node.speed, 0.01f, 0.0f);

				// ループフラグ
				ImGui::Checkbox("Loop", &a_node.isLoop);

				ImGui::PopItemWidth();
			});
	}

	//======================================================================================
	// Animator固有UI: 参照モデル選択
	//======================================================================================
	void StateMachineAsset::BindModelComb()
	{
		std::string _viewName = "Selecte...";
		auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
		if (!_pModel)
		{
			ImGui::Text("Not selected model");
		}
		else
		{
			_viewName = _pModel->GetName();
			ImGui::Text("Model : %s", _viewName.c_str());
		}

		if (ImGui::BeginCombo("Change model", _viewName.c_str()))
		{
			for (auto& _prop : AssetDatabase::Instance().GetTypeMetaVec("Model"))
			{
				bool _selected = (m_modelGUID == _prop.guid);
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					m_modelHandle = ResourceManager::Instance().Load<Model>(_prop.guid);
					m_modelGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}
	}
}
