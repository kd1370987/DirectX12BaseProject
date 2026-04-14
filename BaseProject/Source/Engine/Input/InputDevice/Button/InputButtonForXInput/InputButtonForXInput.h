#pragma once
#include "../InputButtonBase.h"

namespace Engine::Input
{
	class InputButtonForXInput : public InputButtonBase
	{
	public:

		InputButtonForXInput(WORD a_button);
		InputButtonForXInput(const std::vector<WORD>& a_buttonVec);
		~InputButtonForXInput() override = default;

		void Update() override;

		void GetCode(std::vector<int>& a_ret)const override;


	private:

		XINPUT_STATE m_conState = {};
		XINPUT_STATE m_prevConState = {};

		UINT m_userIndex = 0;

		std::vector<WORD> m_buttonVec = {};
	};
}