#include "InputAxisForWindows.h"

#include "../../Button/InputButtonForWindows/InputButtonForWindows.h"

namespace Engine::Input
{
	InputAxisForWindows::InputAxisForWindows(int a_upCode, int a_rightCode, int a_downCode, int a_leftCode)
	{
		m_upDirButtons[ToIndex(EDir::Up)] = std::make_unique<InputButtonForWindows>(a_upCode);
		m_upDirButtons[ToIndex(EDir::Right)] = std::make_unique<InputButtonForWindows>(a_rightCode);
		m_upDirButtons[ToIndex(EDir::Down)] = std::make_unique<InputButtonForWindows>(a_downCode);
		m_upDirButtons[ToIndex(EDir::Left)] = std::make_unique<InputButtonForWindows>(a_leftCode);
	}

	InputAxisForWindows::~InputAxisForWindows() = default;

	void InputAxisForWindows::PreUpdate()
	{
		for (auto& _button : m_upDirButtons)
		{
			_button->PreUpdate();
		}
	}


	void InputAxisForWindows::Update(InputContext& a_inputContext)
	{
		m_axis = Math::Vector2::Zero();
		for (auto& _dirButton : m_upDirButtons)
		{
			_dirButton->Update(a_inputContext);
		}

		if (m_upDirButtons[ToIndex(EDir::Up)]->GetState() != InputButtonBase::EState::Free)		m_axis.y += 1.0f;
		if (m_upDirButtons[ToIndex(EDir::Right)]->GetState() != InputButtonBase::EState::Free)	m_axis.x += 1.0f;
		if (m_upDirButtons[ToIndex(EDir::Down)]->GetState() != InputButtonBase::EState::Free)		m_axis.y -= 1.0f;
		if (m_upDirButtons[ToIndex(EDir::Left)]->GetState() != InputButtonBase::EState::Free)		m_axis.x -= 1.0f;
	}
}