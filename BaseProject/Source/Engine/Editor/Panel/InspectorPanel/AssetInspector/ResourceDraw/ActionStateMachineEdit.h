#pragma once

#include "../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// ゲームプレイ用ステートマシンの編集・詳細表示
	/// ノードエディタ本体はImNodesのコンテキストをアセットが保持しているため、
	/// ActionStateMachineAsset::EditImGuiに委譲している
	/// </summary>
	void ActionStateMachineEdit(
		EditorContext& a_editContext,
		Resource::ActionStateMachineAsset* a_pAsset,
		const Handle<Resource::ActionStateMachineAsset>& a_handle
	);
}
