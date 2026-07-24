#pragma once

#include "../../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// シェーディングモデルテーブルの編集・詳細表示
	/// レンダーパスごとの有効・無効と、登録シェーダーを編集する
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pTable">編集対象のシェーディングモデルテーブル</param>
	void ShadingModelTableEdit(EditorContext& a_editContext, Resource::ShadingModelTable* a_pTable);
}
