#pragma once

namespace App::Game
{
	//------------------------------------------------------------------------------
	// アクション一覧
	//------------------------------------------------------------------------------
	enum class EGameAction : uint32_t
	{
		// プレイヤーアクション
		Look,			// 視点操作
		Move,			// 移動 : 左右前後
		Down,			// 下方向移動
		Boost,			// ブースト : 移動入力がなかったら上方向
		ChargeBoost,	// チャージブースト : ブーストダッシュ中はBoostボタンで上に移動
		RWeaponAttack,	// 右の武器のアタック
		LWeaponAttack,	// 左の武器のアタック
		RMissileLock,	// 右肩のミサイルの照準開始、離すと発射
		LMissileLock,	// 左肩のミサイルの照準開始、離すと発射
		Pose,			// 一時停止

		// UIアクション
		Select,			// 選択
		Back,			// 一つ戻る
		Menu,			// オプションやタイトルに戻るが出てくる
		BottonMove,		// 選択できる項目のみを順番に移動できる
	};

	//------------------------------------------------------------------------------
	// 入力設定
	//------------------------------------------------------------------------------
	// アクション1つに対して、押すキー(ボタン)か4方向のキー(軸)のどちらかを持つ。

	struct ButtonInputData
	{
		int key;
	};

	struct AxisInputData
	{
		int up;
		int right;
		int down;
		int left;
	};

	using InputBindingData = std::variant<ButtonInputData, AxisInputData>;

	struct InputSettings
	{
		std::unordered_map<Game::EGameAction, InputBindingData> keyboard;
		std::unordered_map<Game::EGameAction, InputBindingData> mouse;

		float mouseSensitivity = 1.0f;		// マウス感度
	};
}