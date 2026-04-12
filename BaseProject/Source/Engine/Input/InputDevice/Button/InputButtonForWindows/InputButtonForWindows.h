#pragma once

#include "../InputButtonBase.h"

namespace Engine::Input
{
	// WinAPIのGetAsyncKeyStateを利用したキー制御
	// マウスのボタンとキーボードの入力制御
	class InputButtonForWindows : public InputButtonBase
	{
	public:

		InputButtonForWindows(int a_keyCode);
		InputButtonForWindows(std::initializer_list<int> a_keyCodeList);
		InputButtonForWindows(const std::vector<int>& a_keyCodeList);
		~InputButtonForWindows() override = default;

		void Update() override;

		void GetCode(std::vector<int>& a_ret) const override;

	private:
		std::list<int> m_keyCodeList = {};
	};
}