#pragma once

#include "../../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// モデルの詳細表示
	/// ノード階層・付属アニメーション・メッシュ・マテリアルを表示する
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pModel">表示対象のモデル</param>
	void ModelEdit(EditorContext& a_editContext, Resource::Model* a_pModel);
}
