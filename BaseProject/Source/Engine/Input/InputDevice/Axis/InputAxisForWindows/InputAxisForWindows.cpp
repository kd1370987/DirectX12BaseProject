#include "InputAxisForWindows.h"

#include "../../Button/InputButtonForWindows/InputButtonForWindows.h"

namespace Engine::Input
{
	InputAxisForWindows::InputAxisForWindows(int a_upCode, int a_rightCode, int a_downCode, int a_leftCode)
	{
		// 四つ分用意する
		m_spDirButtons.resize(EDir::Max);

		m_spDirButtons[EDir::Up] = std::make_shared<InputButtonForWindows>(a_upCode);
		m_spDirButtons[EDir::Right] = std::make_shared<InputButtonForWindows>(a_rightCode);
		m_spDirButtons[EDir::Down] = std::make_shared<InputButtonForWindows>(a_downCode);
		m_spDirButtons[EDir::Left] = std::make_shared<InputButtonForWindows>(a_leftCode);
	}

	void InputAxisForWindows::PreUpdate()
	{
		for (auto& _button : m_spDirButtons)
		{
			_button->PreUpdate();
		}
	}


	void InputAxisForWindows::Update()
	{
		m_axis = DXSM::Vector2::Zero;
		for (auto& _dirButton : m_spDirButtons)
		{
			_dirButton->Update();
		}

		if (m_spDirButtons[EDir::Up]->GetState())		m_axis.y += 1.0f;
		if (m_spDirButtons[EDir::Right]->GetState())	m_axis.x += 1.0f;
		if (m_spDirButtons[EDir::Down]->GetState())		m_axis.y -= 1.0f;
		if (m_spDirButtons[EDir::Left]->GetState())		m_axis.x -= 1.0f;
	}
}