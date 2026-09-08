#pragma once

#include "../../Utility/String/StringUtility.h"

//==========================================================================================
// Engine::Input のアクションID
//------------------------------------------------------------------------------------------
// 「どの操作か」を表す番号。ボタン・軸の登録も取得もこの番号を鍵にする。
//
// ゲーム側は操作を enum class : uint32_t で持っているので、その値をそのまま鍵にできる。
// 文字列を渡した場合はハッシュ(Engine::String::ToHash)を通して番号へ直す。
// リテラルならコンパイル時に畳まれるので、実行時のコストは無い。
//==========================================================================================
namespace Engine::Input
{
	// アクションを表す番号
	using ActionID = uint32_t;

	// アクションIDへ直せる列挙型(enum class : uint32_t を想定)
	template<class T>
	concept ActionEnum = std::is_enum_v<T>;

	// 名前として扱える型(文字列リテラル / std::string / std::string_view)
	template<class T>
	concept ActionName = std::is_convertible_v<const T&, std::string_view>;

	// 文字列 -> アクションID
	inline constexpr ActionID ToActionID(std::string_view a_name)
	{
		return static_cast<ActionID>(Engine::String::ToHash(a_name));
	}

	// 列挙値 -> アクションID
	template<ActionEnum T>
	inline constexpr ActionID ToActionID(T a_action)
	{
		return static_cast<ActionID>(a_action);
	}

	/// <summary>
	/// 入力APIの引数として使うアクションの鍵
	/// </summary>
	/// <remarks>
	/// 番号・列挙値・文字列のどれを渡しても受け取れるようにするための型。
	/// これ一つで受けることで、取得関数を三通り並べずに済ませている。
	///
	/// 注意 : 番号の衝突は防げない。
	/// 別々の enum を同じデバイスへ登録すると、どちらも 0 から始まるため
	/// 先頭の項目同士がぶつかる。enum ごとに開始値をずらしておくこと。
	/// </remarks>
	struct ActionKey
	{
		ActionID id = 0;

		// 番号でそのまま指定する
		constexpr ActionKey(ActionID a_id) : id(a_id) {}

		// ゲーム側の enum class をそのまま渡す
		template<ActionEnum T>
		constexpr ActionKey(T a_action) : id(ToActionID(a_action)) {}

		// 名前で指定する(ハッシュを通して番号にする)
		// 文字列リテラル・std::string・std::string_view をまとめてここで受ける。
		// const char* を個別に受けると、数値の 0 が「番号」と「空ポインタ」の
		// どちらとも取れてしまい呼び出しが曖昧になるため、条件付きの一本にしてある
		template<ActionName T>
		constexpr ActionKey(const T& a_name) : id(ToActionID(std::string_view(a_name))) {}

		constexpr operator ActionID() const { return id; }
	};
}
