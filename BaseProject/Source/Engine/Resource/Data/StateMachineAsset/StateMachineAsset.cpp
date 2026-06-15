#include "StateMachineAsset.h"

#include "../../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
UINT Engine::Resource::StateMachineAsset::GetStateHash(const std::string& a_stateName) const
{
	UINT _hash = StringUtility::ToHash(a_stateName);

	auto _it = m_stateNodeMap.find(_hash);
	if (_it != m_stateNodeMap.end())
	{
		return _it->first;
	}
	return INVALID_STATE_HASH;
}

void Engine::Resource::StateMachineAsset::Save(const std::string& a_savePath)
{	
	// 保存直前に最新のノード座標を取得して構造体にセット
	for (auto& [_hash, _node] : m_stateNodeMap)
	{
		ImVec2 _pos = ImNodes::GetNodeEditorSpacePos(_hash);
		_node.editorPos.x = _pos.x;
		_node.editorPos.y = _pos.y;
	}

	auto _dir = FileUtility::GetDirFromPath(a_savePath);
	auto _fileName = FileUtility::GetFileNameWithoutExtension(a_savePath);
	Persistence::Archive _arch(
		Persistence::Archive::Mode::Save,
		_dir,
		_fileName,
		"stet"
	);
	_arch.StringField("m_name",m_name);
	_arch.Field("m_defaultStartHash",m_defaultStartHash);

	// ノード保存
	UINT _size = (UINT)m_stateNodeMap.size();
	_arch.Field("StateNodeMapSize",_size);
	int _i = 0;
	for (auto& [_hash, _node] : m_stateNodeMap)
	{
		std::string _filedName = std::to_string(_i);
		// ノードデータ保存
		_node.Archive(_arch,_filedName);
		_i++;		// フィールド名被り対策
	}

	// 遷移先保存
	_size = (UINT)m_transitionArrowMap.size();
	_arch.Field("TransitionArrowMapSize", _size);
	_i = 0;
	for (auto& [_hash, _arrowVec] : m_transitionArrowMap)
	{
		std::string _filedName = std::to_string(_i);
		// ハッシュ保存
		UINT _h = _hash;
		_arch.Field(_filedName + "Hash",_h);


		// 遷移データ保存
		UINT _arrowSize = (UINT)_arrowVec.size();
		_arch.Field(_filedName + "ArrowVecSize", _arrowSize);
		int _j = 0;
		for (auto& _arrow : _arrowVec)
		{
			auto _arrowFieldName = _filedName + std::to_string(_j);
			_arrow.Archive(_arch,_arrowFieldName);
			_j++;
		}
		_i++;		// フィールド名被り対策
	}

	// 遷移パラメータ
	_size = (UINT)m_parameters.size();
	_arch.Field("m_parametersSize", _size);
	int _k = 0; // 名前被り対策用のインデックス
	for (auto& [_hash, _param] : m_parameters)
	{
		std::string _filedName = std::to_string(_k);

		// マップのキー（ハッシュ値）も保存しておく
		UINT _h = _hash;
		_arch.Field(_filedName + "Hash", _h);

		// パラメータ本体の保存
		_param.Archive(_arch, _filedName);
		_k++;
	}

}

void Engine::Resource::StateMachineAsset::Load(const std::string& a_fileDir, const std::string& a_fileName)
{
	Release();

	Persistence::Archive _arch(
		Persistence::Archive::Mode::Load, 
		a_fileDir,
		a_fileName,
		"stet",
		Persistence::Archive::ArchiveFormat::Json
	);
	_arch.StringField("m_name", m_name);
	_arch.Field("m_defaultStartHash", m_defaultStartHash);

	// ノード保存（読み込み処理）
	UINT _size = 0;
	_arch.Field("StateNodeMapSize", _size);
	for (size_t _i = 0; _i < _size; ++_i)
	{
		StateNode _node;
		std::string _filedName = std::to_string(_i);
		_node.Archive(_arch, _filedName);

		UINT _key = StringUtility::ToHash(_node.name);
		_node.hash = _key;
		m_stateNodeMap.emplace(_key, _node);

		// 読み込んだ座標をImNodesエディタ上に反映する
		ImNodes::SetNodeEditorSpacePos(_key, ImVec2(_node.editorPos.x, _node.editorPos.y));
	}

	// データごとの遷移先
	_size = 0;
	_arch.Field("TransitionArrowMapSize", _size);
	for (size_t _i = 0; _i < _size; ++_i)
	{
		UINT _hash = 0;
		std::string _filedName = std::to_string(_i);
		// ハッシュ保存
		_arch.Field(_filedName + "Hash", _hash);

		UINT _arrowSize = 0;
		_arch.Field(_filedName + "ArrowVecSize", _arrowSize);

		// 遷移データ配列復元
		std::vector<TransitionArrow> _arrowVec = {};
		_arrowVec.resize(_arrowSize);
		for (size_t _j = 0; _j < _arrowSize; ++_j)
		{
			// 遷移データ復元
			auto _arrowFieldName = _filedName + std::to_string(_j);
			TransitionArrow _arrow = {};
			_arrow.Archive(_arch, _arrowFieldName);

			// 配列に追加
			_arrowVec[_j] = _arrow;
		}
		m_transitionArrowMap.emplace(_hash,_arrowVec);
	}

	// 遷移パラメータの復元
	UINT _paramSize = 0;
	_arch.Field("m_parametersSize", _paramSize);
	for (size_t _i = 0; _i < _paramSize; ++_i)
	{
		std::string _filedName = std::to_string(_i);

		UINT _hash = 0;
		_arch.Field(_filedName + "Hash", _hash);

		StateParameter _param;
		_param.Archive(_arch, _filedName);

		m_parameters.emplace(_hash, _param);
	}
}

void Engine::Resource::StateMachineAsset::Release()
{
	m_stateNodeMap.clear();
	m_transitionArrowMap.clear();
}

void Engine::Resource::StateMachineAsset::EditImGui()
{
	// ステートセーブ
	if (ImGui::Button("Save"))
	{
		// ファイルパス取得
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(m_guid);
		Save(_path);
		Editor::MainEditor::Instance().AddLog("%s",_path.c_str());
		Editor::MainEditor::Instance().AddLog(" : Save StateMachinAsset\n");
	}
	ImGui::Separator();

	EditParameters();
	ImGui::Separator();

	// ノード作成
	AddNode();

	// ノードエディター描画
	DrawNodeEditor();

	// リンクの新規作成
	CreateArrow();
}

UINT Engine::Resource::StateMachineAsset::EvaluateNextState(UINT a_currentStateHash, StateMachinInstance& a_instance) const
{
	// 現在のステートから伸びるArrowのリストを取得
	auto _it = m_transitionArrowMap.find(a_currentStateHash);
	if (_it == m_transitionArrowMap.end()) return a_currentStateHash;	// 遷移先がない場合は現在のステートを返す

	// 各Arrowを順にチェック
	for (const auto& _arrow : _it->second)
	{
		bool _allConditionsMet = true;

		// 矢印がもつすべての条件を満たすかどうか
		for (const auto& _cond : _arrow.conditions)
		{
			// パラメーターの定義を取得
			auto _paramIt = m_parameters.find(_cond.paramHash);
			if (_paramIt == m_parameters.end())
			{
				_allConditionsMet = false;
				break;
			}

			const auto& _paramDef = _paramIt->second;
			bool _conditionMet = false;

			// 型ごとに比較処理
			switch (_paramDef.type)
			{
				case EParamType::Float:
				{
					float _val = a_instance.floatParams[_cond.paramHash];
					if (_cond.op == ECompareOp::Greater) _conditionMet = (_val > _cond.thresholdFloat);
					else if (_cond.op == ECompareOp::Less) _conditionMet = (_val < _cond.thresholdFloat);
					break;
				}
				case EParamType::Int:
				{
					int _val = a_instance.intParams[_cond.paramHash];
					if (_cond.op == ECompareOp::Equal) _conditionMet = (_val == _cond.thresholdInt);
					else if (_cond.op == ECompareOp::NotEqual) _conditionMet = (_val != _cond.thresholdInt);
					else if (_cond.op == ECompareOp::Greater) _conditionMet = (_val > _cond.thresholdInt);
					else if (_cond.op == ECompareOp::Less) _conditionMet = (_val < _cond.thresholdInt);
					break;
				}
				case EParamType::Bool:
				case EParamType::Trigger: // Triggerの評価自体はBoolと同じ
				{
					bool _val = a_instance.boolParams[_cond.paramHash];
					if (_cond.op == ECompareOp::True) _conditionMet = (_val == true);
					else if (_cond.op == ECompareOp::False) _conditionMet = (_val == false);
					break;
				}
			}

			if (!_conditionMet)
			{
				_allConditionsMet = false;
				break; // 1つでも条件を満たしていなければこのArrowは破棄
			}	
		}

		// すべての条件を満たした場合、遷移決定
		if (_allConditionsMet)
		{
			for (const auto& _cond : _arrow.conditions)
			{
				if (m_parameters.at(_cond.paramHash).type == EParamType::Trigger)
				{
					a_instance.boolParams[_cond.paramHash] = false;
				}
			}

			// 遷移先ハッシュを返す
			return _arrow.dstStartHash;
		}
	}

	// どの条件も満たさなかった場合は現状維持
	return a_currentStateHash;
}

void Engine::Resource::StateMachineAsset::AddNode()
{
	// ノード追加ボタン
	if (ImGui::Button("Add Node"))
	{
		// ポップアップを開くトリガー
		ImGui::OpenPopup("Add Node Popup");
	}

	// ポップアップを画面中央に表示するための設定
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	// モーダルポップアップの開始（背景のクリックが無効化されます）
	if (ImGui::BeginPopupModal("Add Node Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char _path[256] = "";
		ImGui::InputText("Node Name", _path, sizeof(_path));
		ImGui::Separator();

		// 作成ボタン
		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			if (std::strlen(_path) > 0)
			{
				StateNode node;
				node.hash = StringUtility::ToHash(std::string(_path));
				node.name = std::string(_path);

				m_stateNodeMap[node.hash] = node;

				// 入力欄をクリアしてポップアップを閉じる
				std::memset(_path, 0, sizeof(_path));
				ImGui::CloseCurrentPopup();
			}
		}

		// EnterキーでCreateを押したことにするフォーカス設定
		ImGui::SetItemDefaultFocus();
		ImGui::Separator();

		// キャンセルボタン
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			std::memset(_path, 0, sizeof(_path)); // キャンセル時もクリアする
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Engine::Resource::StateMachineAsset::CreateArrow()
{
	// 遷移Arrowの設定
	int _hoveredLinkId;
	if (ImNodes::IsLinkHovered(&_hoveredLinkId) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		// 編集対象のリンクIDを保持してポップアップをトリガー
		m_editingLinkID = _hoveredLinkId;
		ImGui::OpenPopup("EditConditionPopup");
	}

	// ポップアップ描画
	ArrowPopUp();

	// 遷移Arrowの作成
	int _startAttr, _endAttr;
	if (ImNodes::IsLinkCreated(&_startAttr, &_endAttr))
	{
		UINT _srcHash = 0;
		UINT _dstHash = 0;

		// startAttr と endAttr のピンIDから、対象のノードを探す
		for (const auto& [_hash, _node] : m_stateNodeMap)
		{
			int inPinId = static_cast<int>(_hash);
			int outPinId = static_cast<int>(_hash ^ 0xFFFFFFFF);

			// 引かれた線が「出力ピン(Out)」なら送信元、「入力ピン(In)」なら送信先とする
			if (_startAttr == outPinId || _endAttr == outPinId) _srcHash = _hash;
			if (_startAttr == inPinId || _endAttr == inPinId)  _dstHash = _hash;
		}

		// 両方のノードが正しく見つかった場合のみ登録（Out同士などの不正な接続もここで弾けます）
		if (_srcHash != 0 && _dstHash != 0)
		{
			TransitionArrow _newArrow;

			static int s_linkIdCounter = 10000;
			_newArrow.linkID = s_linkIdCounter++;
			_newArrow.dstStartHash = _dstHash;

			m_transitionArrowMap[_srcHash].push_back(_newArrow);
		}
	}
}

void Engine::Resource::StateMachineAsset::DrawNodeEditor()
{
	// ノードエディター開始
	ImNodes::BeginNodeEditor();


	// 現在のノードとアロウを表示
	for (auto& [_hash, _node] : m_stateNodeMap)
	{
		_node.EditNode();
		auto _it = m_transitionArrowMap.find(_hash);
		if (_it != m_transitionArrowMap.end())
		{
			for (auto& _arrow : _it->second)
			{
				_arrow.EditArrow(_hash);
			}
		}
	}

	// ノードエディター終了
	ImNodes::EndNodeEditor();
}

void Engine::Resource::StateMachineAsset::ArrowPopUp()
{
	if (ImGui::BeginPopup("EditConditionPopup"))
	{
		// 選択されたArrowを検索
		TransitionArrow* _pEditArrow = nullptr;
		std::vector<TransitionArrow>* _pArrowVec = nullptr;

		for (auto& [_hash, _arrowVec] : m_transitionArrowMap)
		{
			for (auto& _arrow : _arrowVec)
			{
				if (m_editingLinkID == _arrow.linkID) 
				{
					_pEditArrow = &_arrow;
					_pArrowVec = &_arrowVec;
					break;
				}
			}
			if (_pEditArrow) break;
		}
		if (!_pEditArrow)
		{
			ImGui::EndPopup();
			return;
		}

		ImGui::Text("Edit Link ID : %d", m_editingLinkID);

		// Arrowの削除ボタン
		if (ImGui::Button("Delete Arrow", ImVec2(90, 0)))
		{
			// 配列から自身を削除
			auto _it = std::remove_if(_pArrowVec->begin(), _pArrowVec->end(),
				[this](const TransitionArrow& a) { return a.linkID == m_editingLinkID; });
			_pArrowVec->erase(_it, _pArrowVec->end());

			// 削除した場合はこれ以上UIを描画せずに閉じる
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}
		ImGui::Separator();

		ImGui::Text("Value");
		ImGui::Separator();

		// 遷移条件編集
		int _uiIndex = 0;	// ID被り防止
		for(auto _it = _pEditArrow->conditions.begin(); _it != _pEditArrow->conditions.end();)
		{
			// UIのパーツ被りを防ぐ
			ImGui::PushID(_uiIndex);

			if (ImGui::Button("x"))
			{
				_it = _pEditArrow->conditions.erase(_it);
				ImGui::PopID();
				continue;
			}
			ImGui::Separator();

			// プレビュー文字を現在のパラメータにする
			const char* _preview = "None";
			if (m_parameters.find(_it->paramHash) != m_parameters.end())
			{
				_preview = m_parameters[_it->paramHash].name.c_str();
			}

			// パラメーターコンボ
			ImGui::SetNextItemWidth(150.0f);
			if(ImGui::BeginCombo("##Param",_preview))
			{
				for (auto& [_hash, _param] : m_parameters)
				{
					bool  _isSelected = (_it->paramHash == _hash);

					if (ImGui::Selectable(_param.name.c_str(), _isSelected))
					{
						_it->paramHash = _hash;
					}

					if (_isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Separator();

			// 遷移条件
			ImGui::SetNextItemWidth(100.0f);
			Editor::DrawEnumCombo("Condition",_it->op);

			ImGui::PopID();
			++_it;
			++_uiIndex;
		}
		ImGui::Separator();

		// 遷移条件の追加
		if (ImGui::Button("Add Condition"))
		{
			_pEditArrow->conditions.emplace_back();
		}

		ImGui::EndPopup();
	}
}

void Engine::Resource::StateMachineAsset::EditParameters()
{
	ImGui::Text("Parameters");
	ImGui::Indent();

	// パラメータ追加ボタン
	if (ImGui::Button("Add Parameter"))
	{
		ImGui::OpenPopup("Add Parameter Popup");
	}

	// ポップアップを画面中央に表示
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Add Parameter Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char _paramName[256] = "";
		static int _paramTypeIdx = 0;
		const char* _typeNames[] = { "Float", "Int", "Bool", "Trigger" };

		ImGui::InputText("New Param Name", _paramName, sizeof(_paramName));
		ImGui::Separator();

		ImGui::Combo("Type", &_paramTypeIdx, _typeNames, IM_ARRAYSIZE(_typeNames));
		ImGui::Separator();

		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			if (std::strlen(_paramName) > 0)
			{
				// パラメータ検索
				UINT _hash = StringUtility::ToHash(_paramName);
				if (m_parameters.find(_hash) == m_parameters.end())
				{
					// なければ新規作成
					StateParameter _newParam;
					_newParam.name = _paramName;
					_newParam.type = static_cast<EParamType>(_paramTypeIdx);
					m_parameters[_hash] = _newParam;

					std::memset(_paramName, 0, sizeof(_paramName));
					ImGui::CloseCurrentPopup();
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			std::memset(_paramName, 0, sizeof(_paramName));
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::Separator();

	// 登録済みパラメータの一覧表示
	if (ImGui::TreeNodeEx("Parameters"))
	{
		const char* _typeNames[] = { "Float", "Int", "Bool", "Trigger" }; // 表示用にもう一度定義
		for (auto& [_hash, _param] : m_parameters)
		{
			ImGui::BulletText("[%s] %s", _typeNames[static_cast<int>(_param.type)], _param.name.c_str());
		}
		ImGui::TreePop();
	}

	ImGui::Unindent();
}

void Engine::Resource::StateNode::Archive(Persistence::Archive& a_arch, const std::string& a_filedName)
{
	// ノード名
	a_arch.StringField(a_filedName + "nodeName",name);

	// ノード位置
	a_arch.Field(a_filedName + "nodePos",editorPos);
	
}

void Engine::Resource::StateNode::EditNode()
{
	// ノード開始
	ImNodes::BeginNode(hash);

	Editor::Node::TitleBar(name);

	// ピンのIDを安全に生成
	int inPinId = static_cast<int>(hash);
	int outPinId = static_cast<int>(hash ^ 0xFFFFFFFF); // ビット反転で全く別のIDにする

	// 入力ピン
	ImNodes::BeginInputAttribute(inPinId);
	ImGui::Text("In");
	ImNodes::EndInputAttribute();

	// 出力ピン
	ImNodes::BeginOutputAttribute(outPinId);
	ImGui::Text("Out");
	ImNodes::EndOutputAttribute();

	// ノード終了
	ImNodes::EndNode();
}

void Engine::Resource::TransitionArrow::Archive(Persistence::Archive& a_arch, const std::string& a_filedName)
{
	a_arch.Field(a_filedName + "linkID", linkID);
	a_arch.Field(a_filedName + "dstStartHash", dstStartHash);

	// 配列のサイズを保存する処理を追加
	UINT _condSize = (UINT)conditions.size();
	a_arch.Field(a_filedName + "ConditionSize", _condSize);
	conditions.resize(_condSize);

	for (int _i = 0; _i < (int)conditions.size(); ++_i)
	{
		auto& _cond = conditions[_i];
		auto _fildName = a_filedName + std::to_string(_i);
		a_arch.Field(_fildName + "paramHash", _cond.paramHash);
		a_arch.Field(_fildName + "op", _cond.op);
		a_arch.Field(_fildName + "thresholdFloat", _cond.thresholdFloat);
		a_arch.Field(_fildName + "thresholdInt", _cond.thresholdInt);
	}
}

void Engine::Resource::TransitionArrow::EditArrow(UINT a_srcHash)
{
	int outPinId = static_cast<int>(a_srcHash ^ 0xFFFFFFFF);
	int inPinId = static_cast<int>(dstStartHash);

	ImNodes::Link(
		linkID,
		outPinId,   // 出力元のピンID
		inPinId     // 入力先のピンID
	);
}

void Engine::Resource::StateParameter::Archive(Persistence::Archive& a_arch, const std::string& a_filedName)
{
	a_arch.StringField(a_filedName + "name", name);
	a_arch.Field(a_filedName + "type", type);
	a_arch.Field(a_filedName + "defaultFloat", defaultFloat);
	a_arch.Field(a_filedName + "defaultInt", defaultInt);
	a_arch.Field(a_filedName + "defaultBool", defaultBool);
}
