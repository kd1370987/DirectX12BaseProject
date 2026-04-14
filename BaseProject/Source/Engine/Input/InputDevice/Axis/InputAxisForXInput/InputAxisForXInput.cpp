#include "InputAxisForXInput.h"

namespace Engine::Input
{
	void InputAxisForXInput::PreUpdate()
	{
		m_prevConState = m_conState;
	}

	void InputAxisForXInput::Update()
	{
		DWORD _res = XInputGetState(m_userIndex, &m_conState);
		if (_res != ERROR_SUCCESS)
		{
			assert(0 && "コントローラー未接続です");
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
