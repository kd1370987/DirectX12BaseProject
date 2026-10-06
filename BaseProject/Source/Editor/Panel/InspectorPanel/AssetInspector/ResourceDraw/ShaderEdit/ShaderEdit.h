#pragma once

#include "../../../../../Internal/EditorContext.h"
#include "Editor/EditorCommon.h"

namespace Editor::Inspector
{
	/// <summary>
	/// シェーダーの詳細表示
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pShader">表示対象のシェーダー</param>
	void ShaderEdit(EditorContext& a_editContext, Resource::Shader* a_pShader);
}
