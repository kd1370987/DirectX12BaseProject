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
	// 入力コンテキストは使用しない
	void InputButtonForXInput::Update(InputContext&)
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

		// Press / Hold / Release の組み立ては基底と同じ
		UpdateState(_isButtonDown);

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