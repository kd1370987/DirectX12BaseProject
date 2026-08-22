#pragma once

#include "../InputButtonBase.h"

namespace Engine::Input
{
	/// <summary>
	/// 修飾キーと組み合わせた同時押し(Ctrl+P など)
	/// </summary>
	/// <remarks>
	/// InputButtonForWindows は登録したキーのどれか1つでも押されていれば押下になる
	/// (同じ操作に複数のキーを割り当てるための作り)。
	/// こちらは逆に「修飾キーを全部押しながら、本命のキーを押したとき」だけ押下にする。
	///
	///     InputButtonForWindowsChord(_p, { VK_CONTROL })  →  Ctrl + P
	///
	/// 押した瞬間(Press)になるのは本命のキーを押したフレームだけなので、
	/// 修飾キーを後から足しても発火しない(Ctrl→P はよくて P→Ctrl はだめ)。
	/// ショートカットとしてはこちらが自然な効き方になる。
	///
	/// 左右どちらの Ctrl でもよいように、VK_CONTROL / VK_SHIFT / VK_MENU を渡すこと
	/// (GetAsyncKeyState はこれらを左右まとめて見てくれる)。
	/// </remarks>
	class InputButtonForWindowsChord : public InputButtonBase
	{
	public:

		/// <param name="a_keyCode">本命のキー</param>
		/// <param name="a_modifierList">押しながらでなければならないキー</param>
		InputButtonForWindowsChord(int a_keyCode, std::initializer_list<int> a_modifierList);
		InputButtonForWindowsChord(int a_keyCode, const std::vector<int>& a_modifierList);
		~InputButtonForWindowsChord() override = default;

		void Update(InputContext& a_inputContext) override;

		void GetCode(std::vector<int>& a_ret) const override;

	private:

		// 本命のキー
		int m_keyCode = 0;

		// 押しながらでなければならないキー
		std::vector<int> m_modifierVec = {};
	};
}
