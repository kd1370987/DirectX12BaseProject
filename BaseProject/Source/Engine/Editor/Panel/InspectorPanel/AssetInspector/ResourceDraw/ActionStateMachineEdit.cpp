#include "ActionStateMachineEdit.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// ゲームプレイ用ステートマシンの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void ActionStateMachineEdit(
		EditorContext& a_editContext,
		Resource::ActionStateMachineAsset* a_pAsset,
		const Handle<Resource::ActionStateMachineAsset>& a_handle
	)
	{
		if (!a_pAsset) { return; }

		// ---- 概要 ----
		ImGui::Text("Name             : %s", a_pAsset->GetName().c_str());

		UINT _defaultStartHash = a_pAsset->GetDefaultStartHash();
		auto _startName = a_pAsset->GetNodeName(_defaultStartHash);
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
		a_pAsset->EditImGui(a_handle);
	}
}
