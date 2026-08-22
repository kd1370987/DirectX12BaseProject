#include "InputButtonForWindowsChord.h"

namespace Engine::Input
{
	InputButtonForWindowsChord::InputButtonForWindowsChord(int a_keyCode, std::initializer_list<int> a_modifierList)
		: m_keyCode(a_keyCode)
		, m_modifierVec(a_modifierList)
	{}

	InputButtonForWindowsChord::InputButtonForWindowsChord(int a_keyCode, const std::vector<int>& a_modifierList)
		: m_keyCode(a_keyCode)
		, m_modifierVec(a_modifierList)
	{}

	//======================================================================================
	// 入力状態の更新
	//--------------------------------------------------------------------------------------
	// 修飾キーが全部押されていて、かつ本命のキーが押されているときだけ押下とする。
	// 修飾キーを離した時点で押下が切れるので、そのフレームが Release になる。
	//
	// ※ 入力コンテキストは使用しない
	//======================================================================================
	void InputButtonForWindowsChord::Update(InputContext&)
	{
		// すでに更新済みなら
		if (!m_needUpdate) return;

		bool _isDown = (GetAsyncKeyState(m_keyCode) & 0x8000) != 0;

		// ひとつでも離れていれば成立しない
		for (int _modifier : m_modifierVec)
		{
			if (!_isDown) break;
			_isDown = (GetAsyncKeyState(_modifier) & 0x8000) != 0;
		}

		// Press/Hold/Release の組み立ては基底に任せる
		UpdateState(_isDown);

		// 更新済み
		m_needUpdate = false;
	}

	void InputButtonForWindowsChord::GetCode(std::vector<int>& a_ret) const
	{
		// 修飾キーも含めて、この操作に関わるキーをすべて返す
		a_ret.push_back(m_keyCode);
		for (int _modifier : m_modifierVec)
		{
			a_ret.push_back(_modifier);
		}
	}
}
