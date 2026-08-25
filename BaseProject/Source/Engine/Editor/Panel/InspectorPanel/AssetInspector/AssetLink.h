#pragma once

#include "../../../Internal/EditorContext.h"

//==========================================================================================
// AssetLink
//
// アセットインスペクターの中から「そのアセットが参照しているアセット」へ飛ぶための部品。
//
// これまで参照先は GUID かファイル名を文字で出しているだけだったので、
// 中身を見るにはアセットパネルで同じ名前を探し直すしかなかった。
// (モデル → マテリアル → テクスチャ のように何段も辿るものは特に面倒)
//
// 飛んだぶんは履歴へ積むので、Back で元のアセットへ戻れる。
//==========================================================================================
namespace Engine::Editor::Inspector
{
	/// <summary>
	/// 指定したアセットをインスペクターで開く
	/// </summary>
	/// <param name="a_isPushHistory">今見ているものを履歴へ積むか(Backで戻る用)</param>
	/// <returns>開けたら true。データベースに無いGUIDなら何もせず false</returns>
	bool JumpToAsset(
		EditorContext& a_editContext,
		const Engine::GUID& a_guid,
		bool a_isPushHistory = true);

	/// <summary>
	/// 参照しているアセットを1件、飛べるリンクとして描く
	/// </summary>
	/// <param name="a_pEditContext">
	/// 飛び先を書き込むコンテキスト。nullptr ならリンクにせず名前だけを出す。
	/// エフェクトエディターのようにインスペクターの選択を持たない画面から
	/// 同じ編集UIを呼んでいるので、飛べない呼び出し方も許している
	/// </param>
	/// <param name="a_label">左に出す項目名。空文字ならリンクだけを出す</param>
	/// <param name="a_guid">参照先のGUID</param>
	/// <param name="a_pDisplayText">
	/// リンクに出す文字。nullptr ならアセット名を使う。
	/// 一覧の行(「[0] Mesh_00」など)を自分で組みたいときに渡す
	/// </param>
	/// <returns>押されたら true(押した時点で飛んでいる)</returns>
	/// <remarks>
	/// 未設定(空のGUID)は薄字で (none)、データベースに無いGUIDは (missing) と出す。
	/// どちらもエラーではなく状態なので、リンクにはせずそのまま見せる。
	/// ImGuiのIDは a_label から作られるので、同じ項目名を並べるときは
	/// 呼ぶ側で PushID すること。
	/// </remarks>
	bool DrawAssetLink(
		EditorContext* a_pEditContext,
		const char* a_label,
		const Engine::GUID& a_guid,
		const char* a_pDisplayText = nullptr);

	/// <summary>
	/// インスペクター上部の行き来バー(Back ボタン＋今いるアセット)
	/// </summary>
	void DrawAssetNavBar(EditorContext& a_editContext);
}
