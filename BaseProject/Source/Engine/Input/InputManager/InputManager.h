#pragma once

#include "../InputCommon.h"
#include "../InputAction.h"

namespace Engine::Input
{
	class IInputDevice;
	class Button;

	class InputManager
	{
	public:

		// アクション追加
		void AddAction(Action a_action,EActionType a_eActionType);

		// アクションに対するボタン追加
		void AddButton(Action a_action,Button a_button);

	private:

		// 登録されているデバイス
		std::vector<IInputDevice> m_inputDeviceVec = {};

		// アクションごとのデータ
		std::unordered_map<Action, ActionData> m_actionDataMap = {};	
	};
}