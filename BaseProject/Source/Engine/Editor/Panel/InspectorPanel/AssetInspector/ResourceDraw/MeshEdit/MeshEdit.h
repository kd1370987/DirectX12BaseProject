#pragma once

#include "../../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// メッシュの詳細表示
	/// メタデータと、実体化している各ドメインデータを表示する
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pMesh">表示対象のメッシュ</param>
	void MeshEdit(EditorContext& a_editContext, Resource::Mesh* a_pMesh);
}
