#pragma once

#include "../IState.h"

namespace App::Object
{
	/// <summary>
	/// 球体状にぐるぐるまとまる動き(小隊長の整理・死亡で使う)
	/// </summary>
	/// <remarks>
	/// ・リーダーは球の表面を進む目標点を追う。水平には一定の速さで回り、
	///   上下(極角)は正弦波で振れるので、体(小隊長・ボイド)は軌跡をなぞって毛糸玉のように巻かれる。
	/// ・球の中心は、入った瞬間に今の進行方向が接線になる位置へ置く(急に折り返さないように)。
	///   高さは「地表から決まった高さ」に置くか、「下端が地表から m_clearance 上になるまで持ち上げるだけ」か。
	/// ・速さは移動入力の長さで上げる(SwarmLeaderMoveSystem は 入力 × moveSpeed)。
	///   小隊長はリーダーの platoonSpeedScale 倍なので、それを超えると列が千切れる。
	/// ・調整値の保存名は使う側が頭に付ける名前 + 項目名(例 : "Reorganize" + "BallRadius")。
	/// </remarks>
	class SwarmBossBallMotion
	{
	public:
		// 球を置く。a_centerHeight が 0 以上なら中心を地表からその高さに置き、
		// 負なら今の高さのまま(下端が地表から m_clearance より下なら持ち上げる)
		void Begin(SwarmBossStateContext& a_context, float a_centerHeight = -1.0f);

		// 1フレーム進めて、リーダーの移動入力を書く。a_radiusScale は半径に掛ける倍率(縮めるとき用)
		void Update(SwarmBossStateContext& a_context, float a_dt, float a_radiusScale = 1.0f);

		// 球のデバッグ表示(赤道と子午線)
		void Draw(SwarmBossStateContext& a_context) const;

		void Archive(Engine::Persistence::Archive& a_ar, const std::string& a_prefix);
		void DrawInspector();

		const Math::Vector3& GetCenter() const { return m_center; }
		float GetRadius() const { return std::max(m_ballRadius, 0.0f) * m_radiusScale; }

	private:
		// 球の表面の目標点と、その速度
		Math::Vector3 CalcTargetPos() const;
		Math::Vector3 CalcTargetVelocity(float a_azimuthSpeed) const;

		// 極角(真上からの角度。ラジアン)とその変化の速さ
		float CalcPolar() const;
		float CalcPolarSpeed() const;

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		float m_ballRadius        = 40.0f;	// 球の半径(m)
		float m_clearance         = 10.0f;	// 球の下端を地表からこれだけ離す(m。高さを決めないとき)
		float m_speedScale        = 1.2f;	// 球の表面を水平に回る速さ(リーダーの移動速度に対する倍率。上下の振れの速さは別に乗る)
		float m_followGain        = 3.0f;	// 目標点からのずれを詰める強さ(1/秒)
		float m_polarAmplitudeDeg = 60.0f;	// 上下の振れ幅(赤道からの角度。度。90で極まで)
		float m_polarPeriod       = 6.0f;	// 上下に1往復する周期(秒。短いと上下の速さが乗って列が千切れる)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		Math::Vector3 m_center = {};		// 球の中心(ワールド)
		float m_azimuth = 0.0f;				// 目標点の方位角(ラジアン。+X から +Z へ)
		float m_time = 0.0f;				// 巻き始めからの経過時間(秒。上下の振れに使う)
		float m_radiusScale = 1.0f;			// 今の半径の倍率
	};
}
