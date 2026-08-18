#pragma once

namespace Engine::Input
{
	class InputCollector;

	// 様々な入力を管理するクラス : 複数のInputCllectorを管理
	class InputManager
	{
	public:

		// 初期化
		void Init();

		// 更新
		// 毎フレーム必須
		void Update();

		/// <summary>
		/// 溜まっている入力状態を捨てる
		/// </summary>
		/// <remarks>
		/// アプリのモード切り替えで呼ぶ。押しっぱなしのボタン・マウスの移動量・
		/// カーソル固定の基準を、切り替えの向こう側へ持ち越さないようにする。
		/// </remarks>
		void ResetInput();

		// マウス制御
		void SetCursorCentered(bool a_enable);		// 毎フレーム中央にマウスを固定
		void SetCursorLock();						// 一度だけマウスを中央に戻す

		// このフレームでカーソル固定が実際に働いているか
		bool IsCursorLockActive() const { return m_isCursorLockActive; }

		/// <summary>
		/// ゲーム側の入力を受け付けてよい状態か
		/// </summary>
		/// <remarks>
		/// プレイモード以外(エディター操作中)やテキスト入力中は false。
		/// ボタンUIのように「入力を見て自分で判定するもの」が、
		/// エディター操作に反応してしまわないよう確認に使う。
		/// </remarks>
		bool IsGameInputEnable() const;

		/// <summary>
		/// カーソルのクライアント領域内の座標を取得する
		/// </summary>
		/// <param name="a_outPos">左上を原点としたクライアント座標(px)</param>
		/// <returns>取得できたら true</returns>
		/// <remarks>
		/// UIの当たり判定はこの座標を描画解像度へ直して使う。
		/// カーソルを中央へ固定している間(IsCursorLockActive)は毎フレーム中央へ
		/// 戻されるので、位置に意味は無い。メニューを出す場面では固定を切ること。
		/// </remarks>
		bool GetCursorClientPos(Math::Vector2& a_outPos) const;

		// すべての有効な入力装置からのボタン入力状態を取得
		short GetButtonState(std::string_view a_name) const;

		bool IsFree(std::string_view a_name) const;
		bool IsPress(std::string_view a_name) const;
		bool IsHold(std::string_view a_name) const;
		bool IsRelease(std::string_view a_name) const;

		// すべての有効な入力装置からの軸入力状態を取得
		DXSM::Vector2 GetAxisState(std::string_view a_name) const;

		// 入力装置の登録
		void AddDevice(std::string_view a_name,InputCollector* a_pInputDevice);
		void AddDevice(std::string_view a_name,std::unique_ptr<InputCollector> a_upInputDevice);

		// アクセサ
		const std::unique_ptr<InputCollector>& GetDevice(std::string_view a_name) const;
		std::unique_ptr<InputCollector>& RefDevice(std::string_view a_name);

		// 解放
		void Release();

	private:

		// クライアント領域の中心をスクリーン座標で取得する
		// ウィンドウの移動・リサイズ・フルスクリーン切替に追従するため毎回実測する
		bool GetClientCenterPos(POINT& a_outPos) const;

	private:

		std::unordered_map<std::string, std::unique_ptr<InputCollector>> m_upInputDeviceMap = {};

		// マウス制御
		bool m_isCursorLockedToCenter = false;	// 固定する設定かどうか
		bool m_isCursorLockActive = false;		// このフレームで実際に固定が働いているか
		bool m_needCursorLockReset = true;		// 固定開始直後 : 基準を取り直し移動量を0にする
		POINT m_lockAnchorPos = {};				// 前フレームに実際にカーソルを移動させたスクリーン座標
		// 移動量(感度を掛けた後)。生のマウス入力(WM_INPUT)から作る。
		// 感度を下げたときに端数を捨てないよう float で持つ
		float m_deltaX = 0.0f;					// 移動量X
		float m_deltaY = 0.0f;					// 移動量Y

	private:
		InputManager();
		~InputManager();
	public:

		static InputManager& Instance()
		{
			static InputManager _instance;
			return _instance;
		}
	};
}