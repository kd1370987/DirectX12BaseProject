#pragma once

#include "../IState.h"
#include "../Common/BallMotion.h"

namespace App::Object
{
	/// <summary>
	/// 死亡 : 地上の高いところで球体状にまとまり、ウェーブをどんどん速めて、最後に全身が爆散する
	/// </summary>
	/// <remarks>
	/// 流れ : まとまる(Gather) → 爆散を頼む → 爆散後(Burst。ここから抜けない)
	///
	/// ・入るかどうかを決めるのはコントローラー(体力が Death Hp 以下になったら、
	///   今の行動を打ち切ってここへ切り替える)。徘徊の攻撃の抽選には入らない。
	/// ・まとまる動きは SwarmBossBallMotion(小隊長の整理と共通)。中心は地表から m_centerHeight の高さ。
	/// ・まとまっている間は、経過の割合 t(0〜1)の2乗で、だんだん速く次のものを変えていく。
	///     ウェーブの速さ … 1 → m_waveSpeedScaleMax 倍(出す間隔も同じだけ縮まる)
	///     球の半径       … 1 → m_endRadiusScale 倍(締まっていく)
	///   ウェーブを走らせるのはコントローラーなので、倍率を頼むだけ(waveSpeedScale)。
	/// ・m_gatherTime が来たら爆散を頼む(burst)。ボイドを球の中心から外へ飛ばし、
	///   しばらくして1体ずつ落とす(死亡エフェクトが出る)のはコントローラーと SwarmBurstSystem。
	///   落ちたぶんだけボスの体力が減り、最後に 0 になる。
	/// ・まとまっている間は体(ボイド)の防御比率を m_defenseRatio(既定 0 = 無敵)にするよう頼む。
	/// </remarks>
	class SwarmBossDeathState : public IState
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
			Gather,		// まとまる : 球の表面を回りながら、ウェーブを速めていく
			Burst,		// 爆散後 : 何もしない(ここから抜けない)
		};

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		SwarmBossBallMotion m_ball;				// まとまる動き(保存名は "Death" + 項目名)
		float m_centerHeight      = 50.0f;		// 球の中心の地表からの高さ(m)
		float m_endRadiusScale    = 0.6f;		// 爆散する直前の球の半径の倍率(締まっていく)

		float m_gatherTime        = 10.0f;		// まとまってから爆散するまでの時間(秒)
		float m_waveSpeedScaleMax = 8.0f;		// 爆散する直前のウェーブの速さの倍率
		float m_defenseRatio      = 0.0f;		// まとまっている間の体の防御比率(0 : 無敵)

		SwarmBossBurstRequest m_burst = {};		// 爆散の調整値(依頼にそのまま写す)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		EPhase m_phase = EPhase::Gather;
		float m_phaseTime = 0.0f;				// 今のフェーズに入ってからの経過時間(秒)
		float m_waveSpeedScale = 1.0f;			// 今頼んでいるウェーブの倍率(表示用)
	};
}
