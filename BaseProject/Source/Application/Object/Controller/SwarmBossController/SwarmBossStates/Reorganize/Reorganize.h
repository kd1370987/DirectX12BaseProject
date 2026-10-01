#pragma once

#include "../IState.h"

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
	/// ・リーダーは球の表面を進む目標点を追う。水平には一定の速さで回り、
	///   上下(極角)は正弦波で振れるので、体(小隊長・ボイド)は軌跡をなぞって毛糸玉のように巻かれる。
	///   球の中心は、入った瞬間に今の進行方向が接線になる位置へ置く(急に折り返さないように)。
	///   地面に埋まらないよう、地表から半径 + m_clearance より下には置かない。
	/// ・まとまり終えたら、コントローラーに小隊長の整理を頼む(isRequestReorganize)。
	///   小隊長を体力の比率まで減らし、ボイドを割り当て直すのはコントローラー。
	///   割り当て直したボイドが新しい小隊長へ寄るまで、落ち着く時間を取ってから徘徊へ戻る。
	/// ・この行動の間は体(ボイド)の防御比率を m_defenseRatio(既定 0 = 無敵)にするよう頼む。
	///   抜けると依頼の既定値(1)に戻るので、元に戻す処理は要らない。
	/// ・速さは移動入力の長さで上げる(SwarmLeaderMoveSystem は 入力 × moveSpeed)。
	///   小隊長はリーダーの platoonSpeedScale 倍なので、それを超えると列が千切れる。
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

		// 球の表面の目標点と、その速度(巻き始めからの経過時間 a_time / 方位角 m_azimuth)
		Math::Vector3 CalcTargetPos(float a_time) const;
		Math::Vector3 CalcTargetVelocity(float a_time, float a_azimuthSpeed) const;

		// 極角(真上からの角度。ラジアン)とその変化の速さ
		float CalcPolar(float a_time) const;
		float CalcPolarSpeed(float a_time) const;

		// 球のデバッグ表示(赤道と子午線)
		void DrawBall(SwarmBossStateContext& a_context) const;

		// 徘徊へ戻す(要求は一度だけ出す)
		void Finish(SwarmBossStateContext& a_context);

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		float m_ballRadius        = 40.0f;	// 球の半径(m)
		float m_clearance         = 10.0f;	// 球の下端を地表からこれだけ離す(m)
		float m_speedScale        = 1.2f;	// 球の表面を水平に回る速さ(リーダーの移動速度に対する倍率。上下の振れの速さは別に乗る)
		float m_followGain        = 3.0f;	// 目標点からのずれを詰める強さ(1/秒)
		float m_polarAmplitudeDeg = 60.0f;	// 上下の振れ幅(赤道からの角度。度。90で極まで)
		float m_polarPeriod       = 6.0f;	// 上下に1往復する周期(秒。短いと上下の速さが乗って列が千切れる)

		float m_gatherTime        = 4.0f;	// まとまる長さ(秒)。過ぎたら整理を頼む
		float m_settleTime        = 2.5f;	// 整理してから徘徊へ戻るまでの長さ(秒)
		float m_defenseRatio      = 0.0f;	// この行動の間の体の防御比率(0 : 無敵)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		EPhase m_phase = EPhase::Gather;
		float m_phaseTime = 0.0f;			// 今のフェーズに入ってからの経過時間(秒)
		float m_time = 0.0f;				// この行動に入ってからの経過時間(秒。上下の振れに使う)

		Math::Vector3 m_center = {};		// 球の中心(ワールド)
		float m_azimuth = 0.0f;				// 目標点の方位角(ラジアン。+X から +Z へ)
		bool m_isRequested = false;			// 整理を頼んだか(表示用)
	};
}
