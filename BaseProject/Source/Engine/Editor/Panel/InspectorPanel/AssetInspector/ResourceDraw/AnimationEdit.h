#pragma once

#include "../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// アニメーションの詳細表示
	/// アニメーションノードごとの各チャンネルのキーを表示する
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pAnimation">表示対象のアニメーション</param>
	void AnimationEdit(EditorContext& a_editContext, Resource::AnimationData* a_pAnimation);
}
