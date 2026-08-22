#pragma once

namespace Engine::Input
{
	struct InputContext;

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
		virtual void Update(InputContext& a_inputContext) = 0;

		// 強制的に入力をなしにする
		void NoInput() { m_state = EState::Free; }

		// アクセサ
		short GetState() const { return m_state; }					// 現在フレームの状態を返す
		virtual void GetCode(std::vector<int>& a_ret) const = 0;	// 入力コードを返す

	protected:

		//==================================================================================
		// 「今押されているか」から状態を進める
		//----------------------------------------------------------------------------------
		// Press/Hold/Release の組み立ては、キー1つでも同時押しでもまったく同じなので
		// ここに置いてある。継承先は「押されているかどうか」を判定して渡すだけでよい。
		//
		//   押している  : 前フレームも押していれば Hold だけ、初めてなら Press + Hold
		//   離している  : 前フレームまで押していれば Release、それ以外はフラグを落とす
		//==================================================================================
		void UpdateState(bool a_isDown)
		{
			// キーが押されていたら
			if (a_isDown)
			{
				// ホールドフラグがついていたらそのフレームに押されたわけではないのでフラグを消す
				if (m_state & EState::Hold)
				{
					m_state &= ~EState::Press;
				}
				// 押されていない状態なら
				else
				{
					m_state |= EState::Press | EState::Hold;
				}
			}
			// キーが押されていないのなら
			else
			{
				// 押されているのなら離されたフレームにする
				if (m_state & EState::Hold)
				{
					m_state &= ~EState::Press;
					m_state &= ~EState::Hold;
					m_state |= EState::Release;
				}
				// 離されたフラグ解除
				else
				{
					m_state &= ~EState::Release;
				}
			}
		}

		// 入力の状態
		short m_state = EState::Free;

		// 重複しての更新を防ぐ
		bool m_needUpdate = true;
	};
}