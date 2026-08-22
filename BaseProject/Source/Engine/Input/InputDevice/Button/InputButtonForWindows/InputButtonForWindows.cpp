#include "InputButtonForWindows.h"

namespace Engine::Input
{
	InputButtonForWindows::InputButtonForWindows(int a_keyCode)
	{
		m_keyCodeList.push_back(a_keyCode);
	}

	InputButtonForWindows::InputButtonForWindows(std::initializer_list<int> a_keyCodeList)
	{
		for (int _keyCode : a_keyCodeList)
		{
			m_keyCodeList.push_back(_keyCode);
		}
	}

	InputButtonForWindows::InputButtonForWindows(const std::vector<int>& a_keyCodeList)
	{
		for (int _keyCode : a_keyCodeList)
		{
			m_keyCodeList.push_back(_keyCode);
		}
	}

	// 入力コンテキストは使用しない
	void InputButtonForWindows::Update(InputContext&)
	{
		// すでに更新済みなら
		if (!m_needUpdate) return;

		// 登録されているキーが押されているかどうか(どれか1つでも押されていれば押下)
		short _keyState = 0;
		for (int _keyCode : m_keyCodeList)
		{
			_keyState |= GetAsyncKeyState(_keyCode);
		}

		// Press/Hold/Release の組み立ては基底に任せる
		UpdateState((_keyState & 0x8000) != 0);

		// 更新済み
		m_needUpdate = false;
	}

	void InputButtonForWindows::GetCode(std::vector<int>& a_ret) const
	{
		// 登録されたすべての入力コードを受け取る
		for (int _code : m_keyCodeList)
		{
			a_ret.push_back(_code);
		}
	}
}
