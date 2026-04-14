#include "InputButtonForXInput.h"
namespace Engine::Input
{
	InputButtonForXInput::InputButtonForXInput(WORD a_button)
	{
		m_buttonVec.push_back(a_button);
	}
	InputButtonForXInput::InputButtonForXInput(const std::vector<WORD>&a_buttonVec)
	{
		m_buttonVec.clear();
		m_buttonVec = a_buttonVec;
	}
	void InputButtonForXInput::Update()
	{
		// 更新済みなら飛ばす
		if (!m_needUpdate) return;

		// 登録されているキーが押されているかどうか
		bool _isButtonDown = false;
		for (auto& _button : m_buttonVec)
		{
			if (m_conState.Gamepad.wButtons & _button)
			{
				_isButtonDown = true;
			}
		}

		// キーが押されていたら
		if (_isButtonDown)
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
	void InputButtonForXInput::GetCode(std::vector<int>&a_ret) const
	{
		for (WORD _code : m_buttonVec)
		{
			a_ret.push_back(static_cast<int>(_code));
		}
	}
}