#include "InputActionManager.h"

#include "../../Core/InputSettings.h"

// エンジン側
#include "../../../../Engine/Input/InputManager/InputManager.h"
#include "Engine/Input/InputCollector/InputCollector.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindowsMouse/InputAxisForWindowsMouse.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForWindows/InputAxisForWindows.h"
#include "Engine/Input/InputDevice/Axis/InputAxisForXInput/InputAxisForXInput.h"

#include "Engine/Input/InputDevice/Button/InputButtonForWindows/InputButtonForWindows.h"
#include "Engine/Input/InputDevice/Button/InputButtonForXInput/InputButtonForXInput.h"
namespace App::Input
{
	void InputActionManager::Init(const Game::UserData& a_data)
	{
		// キーボード設定
		Engine::Input::InputCollector _keyboard;

		auto _addGameAction = [&](Game::EGameAction a_action,int a_keyCode) 
			{
				Engine::Input::InputButtonForWindows _button(a_keyCode);
				_keyboard.AddButton(a_action, std::make_shared<Engine::Input::InputButtonForWindows>(_button));
			};
		auto _addGameAxis = [&](Game::EGameAction a_action, int a_upCode, int a_rightCode, int a_downCode, int a_leftCode)
			{
				Engine::Input::InputAxisForWindows _axis(a_upCode, a_rightCode, a_downCode, a_leftCode);
				_keyboard.AddAxis(a_action, std::make_shared<Engine::Input::InputAxisForWindows>(_axis));
			};

		// ゲームアクションのキーボード設定
		_addGameAxis(Game::EGameAction::Move, 'W', 'D', 'S', 'A');		// 移動
		_addGameAction(Game::EGameAction::Boost, VK_LSHIFT);			// ダッシュ
		_addGameAction(Game::EGameAction::ChargeBoost, VK_SPACE);		// チャージダッシュ
		_addGameAction(Game::EGameAction::Down, VK_LCONTROL);			// 下方向ブースト

		_addGameAction(Game::EGameAction::LWeaponAttack,VK_LBUTTON);	// 左武器攻撃
		_addGameAction(Game::EGameAction::RWeaponAttack,VK_RBUTTON);	// 右武器攻撃

		_addGameAction(Game::EGameAction::LMissileLock, 'Q');			// 左肩ミサイル
		_addGameAction(Game::EGameAction::RMissileLock, 'E');			// 右肩ミサイル

		_addGameAxis(Game::EGameAction::Look, VK_UP, VK_RIGHT, VK_DOWN, VK_LEFT);	// 視点

		// キーボードを登録
		Engine::Input::InputManager::Instance().AddDevice(
			"Keyboard", std::make_unique<Engine::Input::InputCollector>(_keyboard)
		);

		// マウス
		Engine::Input::InputCollector _mouse;
		_mouse.AddAxis(Game::EGameAction::Look, std::make_shared<Engine::Input::InputAxisForWindowsMouse>());

		// マウスを登録
		Engine::Input::InputManager::Instance().AddDevice(
			"Mouse", std::make_unique<Engine::Input::InputCollector>(_mouse)
		);
	}
}