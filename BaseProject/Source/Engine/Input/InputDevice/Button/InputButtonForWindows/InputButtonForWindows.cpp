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

	void InputButtonForWindows::Update()
	{
		// すでに更新済みなら
		if (!m_needUpdate) return;

		// 登録されているキーが押されているかどうか
		short _keyState = 0;
		for (int _keyCode : m_keyCodeList)
		{
			_keyState |= GetAsyncKeyState(_keyCode);
		}

		// キーが押されていたら
		if (_keyState & 0x8000)
		{
			// ホールドフラグがついていたらそのフレームに押されたわけではないのでフラグを消す
			if (m_state & EState::Hold)
			{
				m_state &= ~EState::Press;
			}
			// 押されていない状態なら
			else
			{
				m_state |= EState::Press | EState::Hold;
			}
		}
		// キーが押されていないのなら
		else
		{
			// 押されているのなら離されたフレームにする
			if (m_state & EState::Hold)
			{
				m_state &= ~EState::Press;
				m_state &= ~EState::Hold;
				m_state |= EState::Release;
			}
			// 離されたフラグ解除
			else
			{
				m_state &= ~EState::Release;
			}
		}

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
