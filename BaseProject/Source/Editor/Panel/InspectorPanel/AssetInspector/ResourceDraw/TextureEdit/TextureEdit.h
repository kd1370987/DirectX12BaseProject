#pragma once

#include "../../../../../Internal/EditorContext.h"
#include "Editor/EditorCommon.h"

namespace Editor::Inspector
{
	/// <summary>
	/// テクスチャの詳細表示
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pTexture">表示対象のテクスチャ</param>
	void TextureEdit(EditorContext& a_editContext, Resource::Texture* a_pTexture);
}
