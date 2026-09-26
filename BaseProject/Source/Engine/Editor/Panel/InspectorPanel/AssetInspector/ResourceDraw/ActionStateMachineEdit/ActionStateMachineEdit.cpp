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
		Engine::Editor::Value("Name", "%s", a_pAsset->GetName().c_str());

		UINT _defaultStartHash = a_pAsset->GetDefaultStartHash();
		auto _startName = a_pAsset->GetNodeName(_defaultStartHash);
		if (_startName.empty())
		{
			Engine::Editor::Value("DefaultStart", "(unknown) %u", _defaultStartHash);
		}
		else
		{
			Engine::Editor::Value("DefaultStart", "%s", std::string(_startName).c_str());
		}

		Engine::Editor::Line();

		// ---- ノードエディタ ----
		if (a_editContext.pServices) a_pAsset->EditImGui(a_handle, *a_editContext.pServices);
	}
}
