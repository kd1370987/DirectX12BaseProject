#pragma once

#include "../../../../PanelManager/EditorContext.h"

namespace Engine::Editor::Inspector
{
	// モデル
	void ModelDraw(EditorContext& a_editContext);

	// テクスチャ
	void TextureDraw(EditorContext& a_editContext);

	// ステートマシン
	void AnimatorDraw(EditorContext& a_editContext);
	void ActionStateMachineDraw(EditorContext& a_editContext);

	// パーティクル
	void ParticleDraw(EditorContext& a_editContext);

	// マテリアル
	void MaterialDraw(EditorContext& a_editContext);

	// メッシュ
	void MeshDraw(EditorContext& a_editContext);

	// アニメーション
	void AnimationDraw(EditorContext& a_editContext);

	// シェーダー
	void ShaderDraw(EditorContext& a_editContext);

	// シェーディングモデル
	void ShadingModelTableDraw(EditorContext& a_editContext);

	// プレハブ
	void PrefabDraw(EditorContext& a_editContext);
}