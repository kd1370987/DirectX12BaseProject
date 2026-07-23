#include "AnimatorAsset.h"

#include "../../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	//======================================================================================
	// ノードのArchive
	//======================================================================================
	void AnimatorNode::Archive(Persistence::Archive& a_arch)
	{
		// 共通の「つなぎ情報」(名前・座標・各種ID)
		ArchiveTopology(a_arch);

		// Animator固有: 再生アニメ情報
		a_arch.Field("animGUID", animGUID);
		a_arch.Field("speed", speed);
		a_arch.Field("isLoop", isLoop);
		a_arch.Field("additiveWeight", additiveWeight);
	}

	//======================================================================================
	// 加算ポーズ用ボーン定義のArchive
	//======================================================================================
	void AdditiveBoneDef::Archive(Persistence::Archive& a_arch)
	{
		a_arch.StringField("nodeName", nodeName);
		a_arch.Field("share", share);
		a_arch.Field("axisScale", axisScale);
		a_arch.Field("channel", channel);

		// ハッシュは保存せず、名前から張り直す(モデル差し替えに強くするため)
		if (a_arch.IsLoading())
		{
			nodeNameHash = StringUtility::ToHash(nodeName);
		}
	}

	//======================================================================================
	// 保存
	//======================================================================================
	void AnimatorAsset::Save(const std::string& a_savePath)
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

		// 加算ポーズのボーン定義
		ArchiveAdditiveBones(_arch);
	}

	//======================================================================================
	// 加算ポーズ用ボーン定義のシリアライズ
	//======================================================================================
	void AnimatorAsset::ArchiveAdditiveBones(Persistence::Archive& a_arch)
	{
		size_t _size = m_additiveBones.size();
		if (a_arch.BeginArray("AdditiveBones", _size))
		{
			// ロード時は読み取った要素数に合わせる
			if (a_arch.IsLoading()) m_additiveBones.resize(_size);

			for (size_t _i = 0; _i < _size; ++_i)
			{
				if (a_arch.BeginObject(_i))
				{
					m_additiveBones[_i].Archive(a_arch);
					a_arch.EndObject();
				}
			}
			a_arch.EndArray();
		}
		else if (a_arch.IsLoading())
		{
			// 加算ポーズ導入前に保存されたアセットにはこの項目が無い
			m_additiveBones.clear();
		}
	}

	//======================================================================================
	// 読み込み
	//======================================================================================
	void AnimatorAsset::Load(const std::string& a_fileDir, const std::string& a_fileName)
	{
		LoadInternal(a_fileDir, a_fileName);
	}

	void AnimatorAsset::Load(const std::string& a_filePath)
	{
		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);
		LoadInternal(_dir, _fileName);
	}

	void AnimatorAsset::LoadInternal(const std::string& a_fileDir, const std::string& a_fileName)
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

		// 加算ポーズのボーン定義
		ArchiveAdditiveBones(_arch);

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

	void AnimatorAsset::Release()
	{
		m_graph.Clear();
		m_editor.DestroyContext();
		m_name.clear();
		m_modelGUID = Engine::DefaultGUID;
		m_modelHandle = {};
		m_additiveBones.clear();
	}

	//======================================================================================
	// エディター
	//======================================================================================
	void AnimatorAsset::EditImGui(const Handle<AnimatorAsset>& a_handle)
	{
		// 保存(ファイルパスはハンドル→GUID→パスで解決)
		if (ImGui::Button("Save"))
		{
			auto _guid = ResourceManager::Instance().GetCache<AnimatorAsset>(a_handle);
			auto _path = AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			Save(_path);
			Editor::MainEditor::Instance().AddLog("%s", _path.c_str());
			Editor::MainEditor::Instance().AddLog(" : Save AnimatorAsset\n");
		}
		ImGui::Separator();

		// アニメを付随させるための参照モデル選択(Animator固有)
		BindModelComb();
		ImGui::Separator();

		// 加算ポーズの対象ボーン定義
		AdditiveBoneEdit();
		ImGui::Separator();

		// ノード本体だけ(アニメ選択UI)を注入して汎用ノードエディタを描画
		m_editor.Draw(m_graph,
			[this](AnimatorNode& a_node)
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

				// 加算ポーズの効き(ステートごと)
				ImGui::Text("Additive");
				ImGui::DragFloat("##AdditiveWeight", &a_node.additiveWeight, 0.01f, 0.0f, 1.0f);

				ImGui::PopItemWidth();
			});
	}

	//======================================================================================
	// Animator固有UI: 参照モデル選択
	//======================================================================================
	void AnimatorAsset::BindModelComb()
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

	//======================================================================================
	// Animator固有UI: 加算ポーズの対象ボーン定義
	//======================================================================================
	void AnimatorAsset::AdditiveBoneEdit()
	{
		if (!ImGui::CollapsingHeader("Additive Bones")) return;

		const auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
		if (!_pModel)
		{
			ImGui::TextDisabled("Select a model first");
			return;
		}

		const auto& _nodeVec = _pModel->GetOriginalNodeVec();

		// チャンネルごとの配分合計。1.0から大きく外れていると見た目が破綻するので目安として出す。
		float _shareSum[3] = { 0.0f, 0.0f, 0.0f };
		for (const auto& _def : m_additiveBones)
		{
			size_t _chIdx = static_cast<size_t>(_def.channel);
			if (_chIdx < 3) _shareSum[_chIdx] += _def.share;
		}
		ImGui::Text("Share sum : Aim %.2f / LagArm %.2f / LagLeg %.2f",
			_shareSum[0], _shareSum[1], _shareSum[2]);

		int _removeIdx = -1;
		for (size_t _i = 0; _i < m_additiveBones.size(); ++_i)
		{
			AdditiveBoneDef& _def = m_additiveBones[_i];
			ImGui::PushID(static_cast<int>(_i));

			// 対象ノード選択
			std::string _current = _def.nodeName.empty() ? "Select node..." : _def.nodeName;
			if (ImGui::BeginCombo("Node", _current.c_str()))
			{
				for (const auto& _node : _nodeVec)
				{
					bool _selected = (_def.nodeName == _node.name);
					if (ImGui::Selectable(_node.name.c_str(), _selected))
					{
						_def.nodeName = _node.name;
						_def.nodeNameHash = StringUtility::ToHash(_node.name);
					}
					if (_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// チャンネル選択
			if (ImGui::BeginCombo("Channel", ToString(_def.channel)))
			{
				const EAdditiveChannel _channelVec[] =
				{
					EAdditiveChannel::Aim,
					EAdditiveChannel::LagArm,
					EAdditiveChannel::LagLeg
				};
				for (auto _ch : _channelVec)
				{
					bool _selected = (_def.channel == _ch);
					if (ImGui::Selectable(ToString(_ch), _selected))
					{
						_def.channel = _ch;
					}
					if (_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::DragFloat("Share", &_def.share, 0.01f, 0.0f, 1.0f);

			// Lag系のみ軸ごとの効きを使う(符号を反転させると左右対称にできる)
			if (_def.channel != EAdditiveChannel::Aim)
			{
				ImGui::DragFloat3("AxisScale", &_def.axisScale.x, 0.01f);
			}

			if (ImGui::Button("Remove")) _removeIdx = static_cast<int>(_i);

			ImGui::Separator();
			ImGui::PopID();
		}

		if (_removeIdx >= 0)
		{
			m_additiveBones.erase(m_additiveBones.begin() + _removeIdx);
		}

		if (ImGui::Button("Add Bone"))
		{
			m_additiveBones.emplace_back();
		}
	}
}
