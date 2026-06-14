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


	// ノードエディター開始
	ImNodes::BeginNodeEditor();

	// ノード追加
	static char _path[256] = "";
	ImGui::InputText("NodeName", _path, sizeof(_path));
	if (ImGui::Button("AddNode"))
	{
		StateNode node;

		node.hash = StringUtility::ToHash(std::string(_path));
		node.name = std::string(_path);

		m_stateNodeMap[node.hash] = node;
		std::memset(_path, 0, sizeof(_path));
	}

	// 現在のノードとアロウを表示
	for (auto& [_hash, _node] : m_stateNodeMap)
	{
		_node.EditNode();
		auto _it = m_transitionArrowMap.find(_hash);
		if(_it != m_transitionArrowMap.end())
		{
			for (auto& _arrow : _it->second)
			{
				_arrow.EditArrow(_hash);
			}
		}
	}

	// ノードエディター終了
	ImNodes::EndNodeEditor();

	// ---------------------------------------------------
	// リンクの新規作成
	// ---------------------------------------------------
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
