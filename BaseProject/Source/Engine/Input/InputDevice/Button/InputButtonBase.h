#pragma once

namespace Engine::Input
{
	class InputButtonBase
	{
	public:

		enum EState : short
		{
			Free,					// 入力がない
			Press,					// 押されたフレーム
			Hold = Press << 1,		// 押している間
			Release = Press << 2	// 話されたフレーム
		};

		InputButtonBase() = default;
		virtual ~InputButtonBase() = default;

		// 入力受付状態にする
		void PreUpdate() { m_needUpdate = true; }

		// 入力状態の更新 継承先で必須
		virtual void Update() = 0;

		// 強制的に入力をなしにする
		void NoInput() { m_state = EState::Free; }

		// アクセサ
		short GetState() const { return m_state; }					// 現在フレームの状態を返す
		virtual void GetCode(std::vector<int>& a_ret) const = 0;	// 入力コードを返す

	protected:

		// 入力の状態
		short m_state = EState::Free;

		// 重複しての更新を防ぐ
		bool m_needUpdate = true;
	};
}