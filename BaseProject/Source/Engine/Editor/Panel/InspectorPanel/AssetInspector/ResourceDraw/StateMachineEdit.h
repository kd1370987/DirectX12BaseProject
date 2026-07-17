#pragma once

#include "../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// ステートマシンの編集・詳細表示
	/// ノードエディタ本体はImNodesのコンテキストをアセットが保持しているため、
	/// StateMachineAsset::EditImGuiに委譲している
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pStateMachine">編集対象のステートマシン</param>
	/// <param name="a_handle">編集対象のハンドル</param>
	void StateMachineEdit(
		EditorContext& a_editContext,
		Resource::StateMachineAsset* a_pStateMachine,
		const Handle<Resource::StateMachineAsset>& a_handle
	);
}
