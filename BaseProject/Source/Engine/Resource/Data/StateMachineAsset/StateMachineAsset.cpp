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
	Persistence::Archive _arch(Persistence::Archive::Mode::Save,a_savePath,"stet");
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

void Engine::Resource::StateMachineAsset::Load(const std::string & a_filePath)
{
	Release();

	Persistence::Archive _arch(Persistence::Archive::Mode::Load, a_filePath, "stet", Persistence::Archive::ArchiveFormat::Json);
	_arch.Field("m_defaultStartHash", m_defaultStartHash);

	// ノード保存
	UINT _size = 0;
	_arch.Field("StateNodeMapSize", _size);
	for (size_t _i = 0; _i < _size; ++_i)
	{
		StateNode _node;
		std::string _filedName = std::to_string(_i);
		_node.Archive(_arch,_filedName);

		UINT _key = StringUtility::ToHash(_node.name);
		_node.hash = _key;
		m_stateNodeMap.emplace(_key, _node);
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
		Save(_path +"/"+ m_name);
		Editor::MainEditor::Instance().AddLog("%s",_path);
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
}

void Engine::Resource::StateNode::Archive(Persistence::Archive& a_arch, const std::string& a_filedName)
{
	a_arch.StringField(a_filedName + "nodeName",name);
}

void Engine::Resource::StateNode::EditNode()
{
	// ノード開始
	ImNodes::BeginNode(hash);

	Editor::Node::TitleBar(name);

	// 入力ピン
	ImNodes::BeginInputAttribute(hash * 10 + 1);
	ImGui::Text("In");
	ImNodes::EndInputAttribute();

	// 出力ピン
	ImNodes::BeginInputAttribute(hash * 10 + 2);
	ImGui::Text("out");
	ImNodes::EndOutputAttribute();

	// ノード終了
	ImNodes::EndNode();
}

void Engine::Resource::TransitionArrow::Archive(Persistence::Archive& a_arch, const std::string& a_filedName)
{
	a_arch.Field(a_filedName + "dstStartHash", dstStartHash);
}

void Engine::Resource::TransitionArrow::EditArrow(UINT a_srcHash)
{
	ImNodes::Link(
		linkID,
		a_srcHash * 10 + 2,
		dstStartHash
	);
}
