#pragma once

namespace App::Game
{
	class UserData;
}

namespace App::Input
{
	// 入力デバイス一覧
	enum class EInputDevice
	{
		KeyboardAndMouse,
		Controller
	};
	ENUM_ATTR_BITFLAG(EInputDevice);

	class InputActionManager
	{
	public:

		// ユーザーデータから入力設定情報を復元
		void Init(const Game::UserData& a_data);
		
	private:

		// 現在接続されているデバイス一覧
		EInputDevice m_conectedDevices;

		// 優先されているデバイス : UIの表示用。直近触ったデバイスが登録される
		EInputDevice m_device;
		float m_durationTime = 0.0f;		// 切り替え間隔時間、意図しない入力でちらつきを抑えるため


	};
}