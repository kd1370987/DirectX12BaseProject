#include "StateMachineEdit.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// ステートマシンの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void StateMachineEdit(
		EditorContext& a_editContext,
		Resource::StateMachineAsset* a_pStateMachine,
		const Handle<Resource::StateMachineAsset>& a_handle
	)
	{
		if (!a_pStateMachine) { return; }

		// ---- 概要 ----
		ImGui::Text("Name             : %s", a_pStateMachine->GetName().c_str());

		// 開始ステート名 : ハッシュから引けなければハッシュのまま表示
		UINT _defaultStartHash = a_pStateMachine->GetDefaultStartHash();
		auto _startName = a_pStateMachine->GetNodeName(_defaultStartHash);
		if (_startName.empty())
		{
			ImGui::Text("DefaultStart     : (unknown) %u", _defaultStartHash);
		}
		else
		{
			ImGui::Text("DefaultStart     : %s", std::string(_startName).c_str());
		}

		ImGui::Separator();

		// ---- ノードエディタ ----
		// ImNodesのコンテキストをアセット側が持っているため、描画はアセットに任せる
		a_pStateMachine->EditImGui(a_handle);
	}
}
