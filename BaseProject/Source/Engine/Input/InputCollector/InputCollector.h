#pragma once

namespace Engine::Input
{
	class InputButtonBase;
	class InputAxisBase;

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

		InputCollector() = default;
		~InputCollector() = default;

		void Update();

		// 何かしらの入力を検知したか
		bool IsSomethigInput();

		// 任意の入力状況の取得
		short GetButtonState(std::string_view a_name) const;
		DXSM::Vector2 GetAxisState(std::string_view a_name) const;

		// 入力デバイスの状態の取得と設定
		EActiveState GetActiveState() const { return m_state; }
		void SetActiveState(EActiveState a_state) { m_state = a_state; }

		// アプリケーションボタンの追加・上書き
		void AddButton(std::string_view a_name, InputButtonBase* a_pButton);
		void AddButton(std::string_view a_name, std::shared_ptr<InputButtonBase> a_spButton);

		// 入力軸の追加・上書き
		void AddAxis(std::string_view a_name, InputAxisBase* a_pAxis);
		void AddAxis(std::string_view a_name, std::shared_ptr<InputAxisBase> a_spAxis);

		const std::shared_ptr

	private:

		// 解放
		void Release();

	private:
		// 登録されているデバイス
		std::unordered_map<std::string, std::shared_ptr<InputButtonBase>> m_spButtonMap;
		std::unordered_map<std::string, std::shared_ptr<InputAxisBase>> m_spAxisMap;

		// 有効
		EActiveState m_state = EActiveState::Enable;
	};
}