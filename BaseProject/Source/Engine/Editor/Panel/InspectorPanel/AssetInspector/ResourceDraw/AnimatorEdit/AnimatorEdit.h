#pragma once

#include "../../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// アニメーター(アニメ用ステートマシン)の編集・詳細表示
	/// ノードエディタ本体はImNodesのコンテキストをアセットが保持しているため、
	/// AnimatorAsset::EditImGuiに委譲している
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pAnimator">編集対象のアニメーター</param>
	/// <param name="a_handle">編集対象のハンドル</param>
	void AnimatorEdit(
		EditorContext& a_editContext,
		Resource::AnimatorAsset* a_pAnimator,
		const Handle<Resource::AnimatorAsset>& a_handle
	);
}
