#include "StateMachineAsset.h"

#include "../../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

#include "../../Loader/Model/ModelLoader.h"

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
		ImVec2 _pos = ImNodes::GetNodeEditorSpacePos(_node.nodeID);
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

	// モデル保存
	std::string _guidStr = m_modelGUID.String();
	_arch.StringField("m_modelGUID",_guidStr);
	m_modelGUID.FromString(_guidStr);

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
			auto _arrowFieldName = _filedName + "_" + std::to_string(_j);
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

	std::string _guidStr = "";
	_arch.StringField("m_modelGUID", _guidStr);
	m_modelGUID.FromString(_guidStr);

	m_modelHandle = ModelLoader::Load(m_modelGUID);

	// ノード保存（読み込み処理）
	UINT _size = 0;
	_arch.Field("StateNodeMapSize", _size);
	int _maxId = 0; // ID復旧・更新用
	for (size_t _i = 0; _i < _size; ++_i)
	{
		StateNode _node;
		std::string _filedName = std::to_string(_i);
		_node.Archive(_arch, _filedName);

		UINT _key = StringUtility::ToHash(_node.name);
		_node.hash = _key;

		// 過去のセーブデータ対策（IDが設定されていなければ新規発行し、設定済みなら最大値を記録）
		if (_node.nodeID == 0) _node.nodeID = ++_maxId; 
		else _maxId = std::max(_maxId, _node.nodeID);

		if (_node.inPinID == 0) _node.inPinID = ++_maxId; 
		else _maxId = std::max(_maxId, _node.inPinID);

		if (_node.outPinID == 0) _node.outPinID = ++_maxId; 
		else _maxId = std::max(_maxId, _node.outPinID);

		m_stateNodeMap.emplace(_key, _node);

		// 読み込んだ座標をノードIDを使ってImNodesに反映
		ImNodes::SetNodeEditorSpacePos(_node.nodeID, ImVec2(_node.editorPos.x, _node.editorPos.y));
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
			auto _arrowFieldName = _filedName + "_" + std::to_string(_j);
			TransitionArrow _arrow = {};
			_arrow.Archive(_arch, _arrowFieldName);

			// アローIDの復旧
			if (_arrow.linkID == 0) _arrow.linkID = ++_maxId;
			else _maxId = std::max(_maxId, _arrow.linkID);

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

	m_idCounter = _maxId;
}

std::string_view Engine::Resource::StateMachineAsset::GetNodeName(const UINT& a_hash) const
{
	for (auto& [_hash,_node] : m_stateNodeMap)
	{
		if (_hash == a_hash)
		{
			return _node.name;
		}
	}

	return std::string_view();
}

void Engine::Resource::StateMachineAsset::Release()
{
	m_stateNodeMap.clear();
	m_transitionArrowMap.clear();

	// パラメータや内部カウンターもすべて初期化する
	m_parameters.clear();
	m_defaultStartHash = 0;
	m_editingLinkID = 0;
	m_idCounter = 0;
}

void Engine::Resource::StateMachineAsset::EditImGui()
{
	// ステートセーブ
	if (ImGui::Button("Save"))
	{
		// ファイルパス取得
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(m_guid);
		Save(_path);
		Editor::MainEditor::Instance().AddLog("%s", _path.c_str());
		Editor::MainEditor::Instance().AddLog(" : Save StateMachinAsset\n");
	}

	// リセットボタンをSaveボタンの横に並べる
	ImGui::SameLine();
	RessetButton();
	ImGui::Separator();

	// アニメーションを付随させたい場合、モデルを登録
	BindModelComb();

	// パラメーター編集
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

		// 条件を持たない場合はスキップ
		if (_arrow.conditions.empty())
		{
			continue;
		}
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
				// ▼ 修正: インスタンスにステータスが入っていなければデフォルト値をセット
				if (a_instance.floatParams.find(_cond.paramHash) == a_instance.floatParams.end())
				{
					a_instance.floatParams[_cond.paramHash] = _paramDef.defaultFloat;
				}

				float _val = a_instance.floatParams[_cond.paramHash];
				if (_cond.op == ECompareOp::Greater) _conditionMet = (_val > _cond.thresholdFloat);
				else if (_cond.op == ECompareOp::Less) _conditionMet = (_val < _cond.thresholdFloat);
				else if (_cond.op == ECompareOp::Equal) _conditionMet = (_val == _cond.thresholdFloat);
				else if (_cond.op == ECompareOp::NotEqual) _conditionMet = (_val != _cond.thresholdFloat);
				break;
			}
			case EParamType::Int:
			{
				// ▼ 修正: インスタンスにステータスが入っていなければデフォルト値をセット
				if (a_instance.intParams.find(_cond.paramHash) == a_instance.intParams.end())
				{
					a_instance.intParams[_cond.paramHash] = _paramDef.defaultInt;
				}

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
				// ▼ 修正: インスタンスにステータスが入っていなければデフォルト値をセット
				if (a_instance.boolParams.find(_cond.paramHash) == a_instance.boolParams.end())
				{
					a_instance.boolParams[_cond.paramHash] = _paramDef.defaultBool;
				}

				bool _val = a_instance.boolParams[_cond.paramHash];
				if (_cond.op == ECompareOp::True) _conditionMet = (_val == true);
				else if (_cond.op == ECompareOp::False) _conditionMet = (_val == false);
				// ▼ 念のため追加: UI側で誤って「Equal」「NotEqual」が選ばれてしまった場合のフェイルセーフ
				else if (_cond.op == ECompareOp::Equal) _conditionMet = (_val == true);
				else if (_cond.op == ECompareOp::NotEqual) _conditionMet = (_val == false);
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

				// 新規ノード作成
				node.nodeID = GenerateID();
				node.inPinID = GenerateID();
				node.outPinID = GenerateID();

				// 仮にデフォルトノードがなければ初めに追加されたノードをデフォルトとする
				if (m_defaultStartHash == 0)
				{
					m_defaultStartHash = node.hash;
				}

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

void Engine::Resource::StateMachineAsset::RessetButton()
{
	// ボタンの色を少し赤くして危険な操作であることを視覚的に伝える
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
	if (ImGui::Button("Reset All"))
	{
		// 確認ポップアップのトリガー
		ImGui::OpenPopup("Reset Confirmation Popup");
	}
	ImGui::PopStyleColor(3);

	// リセットの確認ポップアップ
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Reset Confirmation Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to reset the state machine?\nAll nodes, links, and parameters will be permanently deleted.");
		ImGui::Separator();

		// Yesボタン（本当に初期化する）
		if (ImGui::Button("Yes, Reset", ImVec2(120, 0)))
		{
			Release(); // ここで完全にデータをまっさらにする
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		// キャンセルボタン
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
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
			// 引かれた線が「出力ピン(Out)」なら送信元、「入力ピン(In)」なら送信先とする
			if (_startAttr == _node.outPinID || _endAttr == _node.outPinID) _srcHash = _hash;
			if (_startAttr == _node.inPinID || _endAttr == _node.inPinID)  _dstHash = _hash;
		}

		// 両方のノードが正しく見つかった場合のみ登録（Out同士などの不正な接続もここで弾けます）
		if (_srcHash != 0 && _dstHash != 0)
		{
			TransitionArrow _newArrow;

			_newArrow.linkID = GenerateID();
			_newArrow.dstStartHash = _dstHash;

			m_transitionArrowMap[_srcHash].push_back(_newArrow);
		}
	}
}

void Engine::Resource::StateMachineAsset::DrawNodeEditor()
{
	// ノードエディター開始
	ImNodes::BeginNodeEditor();

	// 現在のノードとArrowを描画
	for (auto& [_hash, _node] : m_stateNodeMap)
	{
		// ノード編集
		EditNode(_node);

		// 矢印編集
		auto _it = m_transitionArrowMap.find(_hash);
		if (_it != m_transitionArrowMap.end())
		{
			for (auto& _arrow : _it->second)
			{
				// ▼ 修正: 遷移先のノードを検索し、互いのピンIDを渡して線を引く
				auto _dstNodeIt = m_stateNodeMap.find(_arrow.dstStartHash);
				if (_dstNodeIt != m_stateNodeMap.end())
				{
					_arrow.EditArrow(_node.outPinID, _dstNodeIt->second.inPinID);
				}
			}
		}
	}

	// ノード描画終了
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

		if (m_modelHandle != Handle<Model>())
		{
			ImGui::Separator();
			ImGui::Text("AnimationData");
			ImGui::DragFloat("BlendDuration", &_pEditArrow->blendDuration, 0.01f, 0.0f);
		}

		// 遷移条件編集
		int _uiIndex = 0;	// ID被り防止
		for(auto _it = _pEditArrow->conditions.begin(); _it != _pEditArrow->conditions.end();)
		{

			ImGui::Separator();

			// UIのパーツ被りを防ぐ
			ImGui::PushID(_uiIndex);
			if (ImGui::Button("x"))
			{
				_it = _pEditArrow->conditions.erase(_it);
				ImGui::PopID();
				continue;
			}
			
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
			// 閾値の入力UI
			if (m_parameters.find(_it->paramHash) != m_parameters.end())
			{
				auto& _paramDef = m_parameters[_it->paramHash];
				ImGui::SameLine(); // 横に並べる
				ImGui::SetNextItemWidth(100.0f);

				if (_paramDef.type == EParamType::Float)
				{
					ImGui::InputFloat("##ThresholdF", &_it->thresholdFloat);
				}
				else if (_paramDef.type == EParamType::Int)
				{
					ImGui::InputInt("##ThresholdI", &_it->thresholdInt);
				}
			}
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
					_newParam.hash = _hash;
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

void Engine::Resource::StateMachineAsset::BindModelComb()
{
	// 現在のモデルを取得
	std::string _viewName = "Selecte...";
	auto* _pModel = Resource::ResourceManager::Instance().Get(m_modelHandle);
	if (!_pModel)
	{
		ImGui::Text("Not selected model");
	}
	else
	{
		_viewName = _pModel->GetName();
		ImGui::Text("Model : %s", _viewName.c_str());
	}
	
	// モデル選択画面
	if (ImGui::BeginCombo("Change model", _viewName.c_str()))
	{
		for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("Model"))
		{
			// 現在のモデルと同じGUIDなら選択中フラグを立てる
			bool _selected = (m_modelGUID == _prop.guid);

			// 選択欄
			if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
			{
				// モデルのハンドル取得
				// ロードされていなかったら止まる
				m_modelHandle = Resource::ModelLoader::Request(_prop.filePath);
				m_modelGUID = _prop.guid;
			}
		}
		ImGui::EndCombo();
	}
}

void Engine::Resource::StateMachineAsset::EditNode(StateNode& a_node)
{
	// ノード開始
	ImNodes::BeginNode(a_node.nodeID);

	Editor::Node::TitleBar(a_node.name);

	// 入力ピン
	ImNodes::BeginInputAttribute(a_node.inPinID);
	ImGui::Text("In");
	ImNodes::EndInputAttribute();

	// 出力ピン
	ImNodes::BeginOutputAttribute(a_node.outPinID);
	ImGui::Text("Out");
	ImNodes::EndOutputAttribute();

	// アニメーション変更
	if (m_modelHandle == Handle<Model>())
	{
		ImNodes::EndNode();
		return;
	}
	// モデル取得
	auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
	if (!_pModel)
	{
		ImNodes::EndNode();
		return;
	}

	// アニメーション名取得
	std::string _viewName = "Select...";
	if (a_node.playAnimData != Handle<AnimationData>())
	{
		auto* _pAnimData = ResourceManager::Instance().Get(a_node.playAnimData);
		if (_pAnimData) _viewName = _pAnimData->name;
	}

	// モデルからあにめーしょんを取得
	if (ImGui::BeginCombo("Change Animation", _viewName.c_str()))
	{
		for (auto& _handle : _pModel->GetAnimationHandles())
		{
			auto* _pAnimData = ResourceManager::Instance().Get(_handle);
			
			bool _selected = (a_node.playAnimData == _handle);

			// 選択ラン
			if (ImGui::Selectable(_pAnimData->name.c_str(), _selected))
			{
				a_node.playAnimData = _handle;

			}
		}
		ImGui::EndCombo();
	}

	ImGui::DragFloat("AnimationSpeed", &a_node.speed, 0.01f,0.0f);
	ImGui::Checkbox("IsLoop", &a_node.isLoop);

	// ノード終了
	ImNodes::EndNode();
}

void Engine::Resource::StateNode::Archive(Persistence::Archive& a_arch, const std::string& a_filedName)
{
	// ノード名
	a_arch.StringField(a_filedName + "nodeName",name);

	// ノード位置
	a_arch.Field(a_filedName + "nodePos",editorPos);


	// アニメーションデータ保存
	auto _guidStr = animGUID.String();
	a_arch.StringField(a_filedName + "animGUID", _guidStr);
	animGUID.FromString(_guidStr);
	a_arch.Field(a_filedName + "speed", speed);
	a_arch.Field(a_filedName + "isLoop", isLoop);

	// ノード管理ID
	a_arch.Field(a_filedName + "nodeID", nodeID);
	a_arch.Field(a_filedName + "inPinID", inPinID);
	a_arch.Field(a_filedName + "outPinID", outPinID);
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

	// ブレンドタイム
	a_arch.Field(a_filedName + "blendDuration", blendDuration);
}

void Engine::Resource::TransitionArrow::EditArrow(int a_srcOutPinID, int a_dstInPinID)
{
	// 引数で受け取ったピンIDをそのまま渡す
	ImNodes::Link(
		linkID,
		a_srcOutPinID,  // 出力元のピンID
		a_dstInPinID    // 入力先のピンID
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
