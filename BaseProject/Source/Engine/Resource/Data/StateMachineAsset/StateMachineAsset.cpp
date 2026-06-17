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
	return UINT_MAX;
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

	// ノードのハンドルからGUIDをとってくる
	auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
	if (_pModel)
	{
		for (auto& [_hash, _node] : m_stateNodeMap)
		{
			_node.animGUID = _pModel->GetAnimationGUIDFromHandle(_node.playAnimData);
		}
	}

	// アーカイブ作成
	auto _dir = FileUtility::GetDirFromPath(a_savePath);
	auto _fileName = FileUtility::GetFileNameWithoutExtension(a_savePath);
	Persistence::Archive _arch(Persistence::Archive::Mode::Save,_dir,_fileName,"stet");

	// 保存
	_arch.Field("m_name",m_name);							// ノード名
	_arch.Field("m_defaultStartHash",m_defaultStartHash);	// ノード名ハッシュ値
	_arch.Field("m_modelGUID",m_modelGUID);					// モデルGUID

	// ノード保存
	size_t _nodeSize = m_stateNodeMap.size();
	if (_arch.BeginArray("StateNodes", _nodeSize))
	{
		size_t _idx = 0;
		for (auto& [_hash, _node] : m_stateNodeMap)
		{
			if (_arch.BeginObject(_idx))
			{
				_node.Archive(_arch);
				_arch.EndObject();
			}
			_idx++;
		}
		_arch.EndArray();
	}

	// 遷移先保存
	size_t _arrowMapSize = m_transitionArrowMap.size();
	if (_arch.BeginArray("TransitionArrows", _arrowMapSize))
	{
		// 各遷移情報をノードごとにまとめて保存
		size_t _idx = 0;
		for (auto& [_hash, _arrowVec] : m_transitionArrowMap)
		{
			if (_arch.BeginObject(_idx))
			{
				// どのノードから矢印かを記録
				UINT _h = _hash;
				_arch.Field("SrcHash", _h);

				// 実際の矢印配列を記録
				size_t _arrowSize = _arrowVec.size();
				if (_arch.BeginArray("Arrows", _arrowSize))
				{
					for (size_t _j = 0; _j < _arrowSize; ++_j)
					{
						if (_arch.BeginObject(_j))
						{
							_arrowVec[_j].Archive(_arch);
							_arch.EndObject();
						}
					}
					_arch.EndArray();
				}
				_arch.EndObject();
			}
			_idx++;
		}
		_arch.EndArray();
	}
	
	// 遷移パラメータ
	size_t _paramSize = m_parameters.size();
	if (_arch.BeginArray("Parameters", _paramSize))
	{
		// パラメータを配列で保存
		size_t _idx = 0;
		for (auto& [_hash, _param] : m_parameters)
		{
			if (_arch.BeginObject(_idx))
			{
				UINT _h = _hash;
				_arch.Field("Hash", _h);
				_param.Archive(_arch);
				_arch.EndObject();
			}
			_idx++;
		}
		_arch.EndArray();
	}

}

void Engine::Resource::StateMachineAsset::Load(const std::string& a_fileDir, const std::string& a_fileName)
{
	Release();

	Persistence::Archive _arch(Persistence::Archive::Mode::Load, a_fileDir, a_fileName, "stet", Persistence::Archive::ArchiveFormat::Json);

	_arch.Field("m_name", m_name);
	_arch.Field("m_defaultStartHash", m_defaultStartHash);

	_arch.Field("m_modelGUID", m_modelGUID);
	m_modelHandle = ModelLoader::Load(m_modelGUID);

	int _maxId = 0;

	// ノード復元
	size_t _nodeSize = 0;
	if (_arch.BeginArray("StateNodes", _nodeSize))
	{
		for (size_t _i = 0; _i < _nodeSize; ++_i)
		{
			if (_arch.BeginObject(_i))
			{
				StateNode _node;
				_node.Archive(_arch);

				UINT _key = StringUtility::ToHash(_node.name);
				_node.hash = _key;

				// ノードID,入力ID,出力ID
				if (_node.nodeID == 0) _node.nodeID = ++_maxId; else _maxId = std::max(_maxId, _node.nodeID);
				if (_node.inPinID == 0) _node.inPinID = ++_maxId; else _maxId = std::max(_maxId, _node.inPinID);
				if (_node.outPinID == 0) _node.outPinID = ++_maxId; else _maxId = std::max(_maxId, _node.outPinID);

				m_stateNodeMap.emplace(_key, _node);
				ImNodes::SetNodeEditorSpacePos(_node.nodeID, ImVec2(_node.editorPos.x, _node.editorPos.y));

				_arch.EndObject();
			}
		}
		_arch.EndArray();
	}

	// 矢印復元
	size_t _arrowMapSize = 0;
	if (_arch.BeginArray("TransitionArrows", _arrowMapSize))
	{
		for (size_t _i = 0; _i < _arrowMapSize; ++_i)
		{
			if (_arch.BeginObject(_i))
			{
				UINT _hash = 0;
				_arch.Field("SrcHash", _hash);

				size_t _arrowSize = 0;
				std::vector<TransitionArrow> _arrowVec;

				if (_arch.BeginArray("Arrows", _arrowSize))
				{
					_arrowVec.resize(_arrowSize);
					for (size_t _j = 0; _j < _arrowSize; ++_j)
					{
						if (_arch.BeginObject(_j))
						{
							_arrowVec[_j].Archive(_arch);

							if (_arrowVec[_j].linkID == 0) _arrowVec[_j].linkID = ++_maxId;
							else _maxId = std::max(_maxId, _arrowVec[_j].linkID);

							_arch.EndObject();
						}
					}
					_arch.EndArray();
				}
				m_transitionArrowMap.emplace(_hash, _arrowVec);
				_arch.EndObject();
			}
		}
		_arch.EndArray();
	}

	// パラメータ復元
	size_t _paramSize = 0;
	if (_arch.BeginArray("Parameters", _paramSize))
	{
		for (size_t _i = 0; _i < _paramSize; ++_i)
		{
			if (_arch.BeginObject(_i))
			{
				UINT _hash = 0;
				_arch.Field("Hash", _hash);

				StateParameter _param;
				_param.Archive(_arch);
				m_parameters.emplace(_hash, _param);

				_arch.EndObject();
			}
		}
		_arch.EndArray();
	}

	m_idCounter = _maxId;

	// モデルからノードのアニメーションを復元
	m_modelHandle = ModelLoader::Load(m_modelGUID);
	auto* _pModel = ResourceManager::Instance().Get(m_modelHandle);
	if (_pModel)
	{
		for (auto& [_hash, _node] : m_stateNodeMap)
		{
			_node.playAnimData = _pModel->GetAnimationHandleFromGUID(_node.animGUID);
		}
	}
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

const Engine::Resource::StateNode* Engine::Resource::StateMachineAsset::GetStateNode(UINT a_stateHash) const 
{
	auto _it = m_stateNodeMap.find(a_stateHash);
	if (_it != m_stateNodeMap.end())
	{
		return &_it->second;
	}
	return nullptr;
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
				// インスタンスにステータスが入っていなければデフォルト値をセット
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
				// インスタンスにステータスが入っていなければデフォルト値をセット
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
				// インスタンスにステータスが入っていなければデフォルト値をセット
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

void Engine::Resource::StateNode::Archive(Persistence::Archive& a_arch)
{
	// ノード名
	a_arch.StringField("nodeName",name);

	// ノード位置
	a_arch.Field("nodePos",editorPos);

	// アニメーションデータ保存
	a_arch.Field("animGUID", animGUID);
	a_arch.Field("speed", speed);
	a_arch.Field("isLoop", isLoop);

	// ノード管理ID
	a_arch.Field("nodeID", nodeID);
	a_arch.Field("inPinID", inPinID);
	a_arch.Field("outPinID", outPinID);
}


void Engine::Resource::TransitionArrow::Archive(Persistence::Archive& a_arch)
{
	a_arch.Field("linkID", linkID);
	a_arch.Field("dstStartHash", dstStartHash);
	a_arch.Field("blendDuration", blendDuration);

	// 配列開始
	size_t _condSize = conditions.size();
	a_arch.Field("ConditionSize", _condSize);
	conditions.resize(_condSize);
	if (a_arch.BeginArray("Conditions", _condSize))
	{
		conditions.resize(_condSize);
		for (size_t _i = 0; _i < _condSize; ++_i)
		{
			// オブジェクト開始
			if (a_arch.BeginObject(_i))
			{
				auto& _cond = conditions[_i];
				a_arch.Field("paramHash", _cond.paramHash);
				a_arch.Field("op", _cond.op);
				a_arch.Field("thresholdFloat", _cond.thresholdFloat);
				a_arch.Field("thresholdInt", _cond.thresholdInt);
				a_arch.EndObject();
			}
		}
		a_arch.EndArray();
	}
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

void Engine::Resource::StateParameter::Archive(Persistence::Archive& a_arch)
{
	a_arch.StringField("name", name);
	a_arch.Field("type", type);
	a_arch.Field("defaultFloat", defaultFloat);
	a_arch.Field("defaultInt", defaultInt);
	a_arch.Field("defaultBool", defaultBool);
}
