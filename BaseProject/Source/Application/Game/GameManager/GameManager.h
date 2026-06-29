#pragma once
namespace App::Game
{
	class GameFlowStateMachine;
	
	/// <summary>
	/// シーンを跨いで保持したいゲーム情報
	/// </summary>
	struct GlobalGameData
	{
		
	};

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
		const GlobalGameData& GetGameData() const { return m_gameData; }
		GlobalGameData& RefGameData() { return m_gameData; }

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

		// グローバルデータ
		GlobalGameData m_gameData;


		// 現在のフローのハッシュ値
		UINT m_currentFlowHash = 0;

		// 一時停止フラグ（ポーズ画面用）
		bool m_isPaused = false;

		// ゲームフロウ管理用
		std::unique_ptr<GameFlowStateMachine> m_upGameFlowMachine = nullptr;

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