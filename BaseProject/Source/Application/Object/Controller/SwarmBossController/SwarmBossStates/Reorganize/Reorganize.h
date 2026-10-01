#pragma once

#include "../IState.h"
#include "../Common/BallMotion.h"

namespace App::Object
{
	/// <summary>
	/// 小隊長の整理 : 体力が一定量減るたびに、球体状にぐるぐるまとまって小隊長を減らす
	/// </summary>
	/// <remarks>
	/// 流れ : まとまる(Gather) → 整理を頼む → 落ち着く(Settle) → 徘徊へ
	///
	/// ・入るかどうかを決めるのはコントローラー(体力が Reorganize Hp Interval 減るたびに、
	///   今の行動を打ち切ってここへ切り替える)。徘徊の攻撃の抽選には入らない。
	/// ・まとまる動きは SwarmBossBallMotion(死亡と共通)。高さは今のまま、地面に埋まる分だけ持ち上げる。
	/// ・まとまり終えたら、コントローラーに小隊長の整理を頼む(isRequestReorganize)。
	///   小隊長を体力の比率まで減らし、ボイドを割り当て直すのはコントローラー。
	///   割り当て直したボイドが新しい小隊長へ寄るまで、落ち着く時間を取ってから徘徊へ戻る。
	/// ・この行動の間は体(ボイド)の防御比率を m_defenseRatio(既定 0 = 無敵)にするよう頼む。
	///   抜けると依頼の既定値(1)に戻るので、元に戻す処理は要らない。
	/// </remarks>
	class SwarmBossReorganizeState : public IState
	{
	public:
		void Enter(SwarmBossStateContext& a_context) override;
		void Update(SwarmBossStateContext& a_context) override;
		void Exit(SwarmBossStateContext& a_context) override;

		void Archive(Engine::Persistence::Archive& a_ar) override;
		void DrawInspector() override;

	private:
		enum class EPhase
		{
			Gather,		// まとまる : 球の表面を回りながら体を巻く
			Settle,		// 落ち着く : 整理を頼んだ後、ボイドが新しい小隊長へ寄るのを待つ(回り続ける)
			End,		// 切り替え要求済み(次のフレームで抜ける)
		};

		// 次のフェーズへ(経過時間は0から数え直す)
		void ChangePhase(EPhase a_phase);

		// 徘徊へ戻す(要求は一度だけ出す)
		void Finish(SwarmBossStateContext& a_context);

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		SwarmBossBallMotion m_ball;			// まとまる動き(保存名は "Reorganize" + 項目名)

		float m_gatherTime   = 4.0f;		// まとまる長さ(秒)。過ぎたら整理を頼む
		float m_settleTime   = 2.5f;		// 整理してから徘徊へ戻るまでの長さ(秒)
		float m_defenseRatio = 0.0f;		// この行動の間の体の防御比率(0 : 無敵)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		EPhase m_phase = EPhase::Gather;
		float m_phaseTime = 0.0f;			// 今のフェーズに入ってからの経過時間(秒)
		bool m_isRequested = false;			// 整理を頼んだか(表示用)
	};
}
