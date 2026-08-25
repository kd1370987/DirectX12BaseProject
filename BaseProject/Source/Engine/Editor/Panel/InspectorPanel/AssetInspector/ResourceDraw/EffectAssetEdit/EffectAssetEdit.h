#pragma once

#include "../../../../../Internal/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// エフェクトアセットの編集・詳細表示
	/// パーティクル・メッシュ・サウンドのパーツを足し引きして、1つのエフェクトに組み上げる
	/// </summary>
	/// <param name="a_guid">編集対象のGUID(保存先を引くのに使う)</param>
	/// <param name="a_pEffect">編集対象のエフェクト</param>
	/// <param name="a_isShowOpenEditorButton">
	/// 「Open Effect Editor」を出すか。
	/// エフェクトエディターの中から呼ぶときは自分自身を開くボタンになるので false
	/// </param>
	/// <param name="a_pEditContext">
	/// 参照しているアセット(パーティクル・モデル・サウンド)を、押すと飛べるリンクにする。
	/// nullptr なら名前を出すだけ
	/// </param>
	/// <remarks>
	/// アセットインスペクターとエフェクトエディターの両方から呼ばれる。
	/// 同じ中身を2箇所で書くと片方だけ直し忘れるので、editContext は必須にしていない
	/// (エフェクトエディターはインスペクターの選択を持たないため渡せない)
	/// </remarks>
	void EffectAssetEdit(
		const Engine::GUID& a_guid,
		Resource::EffectAsset* a_pEffect,
		bool a_isShowOpenEditorButton = true,
		EditorContext* a_pEditContext = nullptr);
}
