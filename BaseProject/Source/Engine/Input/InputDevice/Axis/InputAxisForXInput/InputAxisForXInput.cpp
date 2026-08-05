#include "InputAxisForXInput.h"

namespace Engine::Input
{
	void InputAxisForXInput::PreUpdate()
	{
		m_prevConState = m_conState;
	}

	// 入力コンテキストは使用しない
	void InputAxisForXInput::Update(InputContext&)
	{
		DWORD _res = XInputGetState(m_userIndex, &m_conState);
		if (_res != ERROR_SUCCESS)
		{
			ENGINE_ERRLOG(false,"コントローラーが未接続です");
			return;
		}

		if(m_isLeft)
		{
			m_axis.x = (float)(m_conState.Gamepad.sThumbLX / 32767.f);
			m_axis.y = (float)(m_conState.Gamepad.sThumbLY / 32767.f);
		}
		else
		{
			m_axis.x = (float)(m_conState.Gamepad.sThumbRX / 32767.f);
			m_axis.y = (float)(m_conState.Gamepad.sThumbRY / 32767.f);
		}
	}
}
