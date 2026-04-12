#pragma once

#include "../InputAxisBase.h"

namespace Engine::Input
{
	class InputButtonBase;

	/// <summary>
	/// WinAPIのGetAsyncKeyStateの入力を利用した軸制御
	/// 指定した上下左右のキーの入力状況を軸情報として保持する
	/// </summary>
	class InputAxisForWindows : public InputAxisBase
	{
	public:

		InputAxisForWindows(int a_upCode, int a_rightCode, int a_downCode, int a_leftCode);

		void PreUpdate() override;
		void Update() override;

	private:
		enum EDir
		{
			Up,
			Right,
			Down,
			Left,
			Max
		};
		std::vector<std::shared_ptr<InputButtonBase>> m_spDirButtons;
	};
}