#pragma once

#include "../../../../../Internal/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// エフェクトアセットの編集・詳細表示
	/// パーティクルとメッシュのパーツを足し引きして、1つのエフェクトに組み上げる
	/// </summary>
	/// <param name="a_guid">編集対象のGUID(保存先を引くのに使う)</param>
	/// <param name="a_pEffect">編集対象のエフェクト</param>
	/// <param name="a_isShowOpenEditorButton">
	/// 「Open Effect Editor」を出すか。
	/// エフェクトエディターの中から呼ぶときは自分自身を開くボタンになるので false
	/// </param>
	/// <remarks>
	/// アセットインスペクターとエフェクトエディターの両方から呼ばれる。
	/// 同じ中身を2箇所で書くと片方だけ直し忘れるので、editContext には依存させていない
	/// </remarks>
	void EffectAssetEdit(
		const Engine::GUID& a_guid,
		Resource::EffectAsset* a_pEffect,
		bool a_isShowOpenEditorButton = true);
}
