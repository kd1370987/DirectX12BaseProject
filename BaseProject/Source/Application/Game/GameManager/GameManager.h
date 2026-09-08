#pragma once

#include "../GlobalGameContext.h"

namespace App::Input
{
	class InputActionManager;
}

namespace App::Game
{
	class UserData;

	/// <summary>
	/// シングルトン
	/// ゲーム全体を通しての流れを管理するクラス
	/// </summary>
	class GameManager
	{
	public:

		/// <summary>
		/// 初期化 : ゲーム起動時の一度のみ呼ばれる
		/// </summary>
		void Init();

		/// <summary>
		/// メインループからマイフレーム呼ばれる
		/// </summary>
		void Update(float a_dt);

		/// <summary>
		/// メインループから呼ばれる : 命令を積むだけで実行はしない
		/// </summary>
		void Draw();

		/// <summary>
		/// ゲーム終了処理
		/// </summary>
		void Release();

		// ---- アクセサ ----
		const GlobalGameContext& GetGameData() const { return m_gameData; }
		GlobalGameContext& RefGameData() { return m_gameData; }

		/// <summary>
		/// ゲーム内イベントの発火（UIのボタンやシステムから呼ばれる） 
		/// </summary>
		/// <param name="a_eventName">イベント名</param>
		void FireGlobalEvent(const std::string& a_eventName);

		/// <summary>
		/// エディター描画用
		/// </summary>
		void EditDraw();

	private:

		/// <summary>
		/// ゲーム設定の読み込み : Init から一度だけ
		/// </summary>
		void LoadGameSetting();

		/// <summary>
		/// ゲーム設定の保存 : エディターの Save ボタンから
		/// </summary>
		void SaveGameSetting();

		/// <summary>
		/// ゲーム設定の編集UI(起動時に立ち上げるシーンを選ぶ)
		/// </summary>
		void DrawGameSettingEdit();

	private:

		// シーンをまたいで保持できる情報
		GlobalGameContext m_gameData;

		// 一時停止フラグ（ポーズ画面用）
		bool m_isPaused = false;

		// ゲーム開始時の初回シーン : 起動時に出現させる
		Engine::GUID m_farstScene;

		Engine::Handle<Engine::Resource::SoundInstance> m_testHandle = { };

		// ユーザーデータ
		std::unique_ptr<UserData> m_upUserData;

		// ---- 設定 ----
		// 入力
		std::unique_ptr<Input::InputActionManager> m_upInputActionManager;

	// シングルトン
	private:
		GameManager();
		~GameManager();

		// ムーブコピー禁止
		GameManager(const GameManager&) = delete;
		GameManager& operator=(const GameManager&) = delete;
		GameManager(GameManager&&) noexcept = default;
		GameManager& operator=(GameManager&&) noexcept = default;

	public:

		static GameManager& Instance()
		{
			static GameManager _instance;
			return _instance;
		}
	};
}