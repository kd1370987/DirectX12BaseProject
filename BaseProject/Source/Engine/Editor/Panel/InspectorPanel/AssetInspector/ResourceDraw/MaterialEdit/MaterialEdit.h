#pragma once

#include "../../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// マテリアルの編集・詳細表示
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pMaterial">編集対象のマテリアル</param>
	void MaterialEdit(EditorContext& a_editContext, Resource::Material* a_pMaterial);
}
