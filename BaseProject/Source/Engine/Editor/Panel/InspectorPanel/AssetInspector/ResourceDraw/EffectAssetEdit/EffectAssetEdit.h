#pragma once

#include "../../../../../Internal/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// エフェクトアセットの編集・詳細表示
	/// パーティクルとメッシュのパーツを足し引きして、1つのエフェクトに組み上げる
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pEffect">編集対象のエフェクト</param>
	void EffectAssetEdit(EditorContext& a_editContext, Resource::EffectAsset* a_pEffect);
}
