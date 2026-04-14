#pragma once

#include "../InputAxisBase.h"

namespace Engine::Input
{
	class InputButtonBase;

	/// <summary>
	/// XInputを使用したコントローラー軸制御
	/// </summary>
	class InputAxisForXInput : public InputAxisBase
	{
	public:

		InputAxisForXInput(UINT a_userIndex,bool a_isLeft) 
		{
			m_userIndex = a_userIndex;
			m_isLeft = a_isLeft;
		};

		void PreUpdate() override;

		// フレームの移動量を使って、軸の入力状況を更新
		void Update() override;

	private:
		// 左かどうか
		bool m_isLeft = false;

		// 今は個別で持ってるが、コントローラークラスができたときにでもポインタに変更予定
		XINPUT_STATE m_conState = {};
		XINPUT_STATE m_prevConState = {};

		UINT m_userIndex = 0;
	};
}