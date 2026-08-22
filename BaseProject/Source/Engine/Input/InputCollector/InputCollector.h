#pragma once

namespace Engine::Input
{
	class InputButtonBase;
	class InputAxisBase;

	struct InputContext;

	/// <summary>
	/// 単一の入力デバイスからの入力をコレクションするクラス
	/// キーボードやゲームパッド等のそれぞれのInputCollectorが必要
	/// ゲームで使う入力Indexの管理もここで行う
	/// </summary>
	class InputCollector
	{
	public:

		enum class EActiveState
		{
			Disable,		// 無効 : 完全に停止している状態
			Monitoring,		// 監視 : デバイスの入力を更新、アプリに入力の影響はない
			Enable,			// 有効 : アプリに入力の影響を与える
		};

		InputCollector();
		~InputCollector();

		void Update(InputContext& a_inputContext);

		/// <summary>
		/// このデバイスに溜まっている入力状態を捨てる
		/// </summary>
		/// <remarks>
		/// アプリのモード切り替えで呼ぶ。押しっぱなし(Hold)のまま切り替えると、
		/// 戻ってきたときに「押した瞬間(Press)」を挟まずに Hold から始まってしまう。
		/// </remarks>
		void ResetInput();

		// 何かしらの入力を検知したか
		bool IsSomethigInput();

		// 任意の入力状況の取得
		short GetButtonState(std::string_view a_name) const;
		DXSM::Vector2 GetAxisState(std::string_view a_name) const;

		// 入力デバイスの状態の取得と設定
		EActiveState GetActiveState() const { return m_state; }
		void SetActiveState(EActiveState a_state) { m_state = a_state; }

		/// <summary>
		/// モード切り替えの入力リセットで捨てないデバイスにする
		/// </summary>
		/// <remarks>
		/// リセットは「切り替え前の操作を向こう側へ持ち越さない」ためのものなので、
		/// ゲームの操作は捨ててよい。
		///
		/// ただし切り替えそのものに使うキー(Ctrl+P)を捨てると、押しっぱなしの状態が
		/// 「押した瞬間」へ戻り、押している間ずっと切り替わり続けてしまう。
		/// システム用のデバイスはこれを立てて、リセットの対象から外す。
		/// </remarks>
		void SetKeepOnReset(bool a_isKeep) { m_isKeepOnReset = a_isKeep; }
		bool IsKeepOnReset() const { return m_isKeepOnReset; }

		// アプリケーションボタンの追加・上書き
		void AddButton(std::string_view a_name, InputButtonBase* a_pButton);
		void AddButton(std::string_view a_name, std::shared_ptr<InputButtonBase> a_spButton);

		// 入力軸の追加・上書き
		void AddAxis(std::string_view a_name, InputAxisBase* a_pAxis);
		void AddAxis(std::string_view a_name, std::shared_ptr<InputAxisBase> a_spAxis);

		const std::shared_ptr<InputButtonBase> GetButton(std::string_view a_name)  const;
		const std::shared_ptr<InputAxisBase> GetAxis(std::string_view a_name) const;

	private:

		// 解放
		void Release();

	private:
		// 登録されているデバイス
		std::unordered_map<std::string, std::shared_ptr<InputButtonBase>> m_spButtonMap;
		std::unordered_map<std::string, std::shared_ptr<InputAxisBase>> m_spAxisMap;

		// 有効
		EActiveState m_state = EActiveState::Enable;

		// モード切り替えのリセットで捨てないか(システム用の入力だけ true)
		bool m_isKeepOnReset = false;
	};
}