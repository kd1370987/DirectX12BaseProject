#pragma once

#include "../IState.h"

namespace App::Object
{
	/// <summary>
	/// ダイブ : プレイヤーから離れて高く上がり、放物線(二次関数)を描いてプレイヤーへ急降下する
	/// </summary>
	/// <remarks>
	/// 流れ : 離れる(Rise) → 狙う(Hover) → 急降下(Dive) → 余韻(Recover) → 徘徊へ
	///
	/// ・離れる先はプレイヤーから見て今の自分側へ m_launchDistance、高さ m_launchHeight の点。
	/// ・狙う間はゆっくりプレイヤーへ寄る(向きを変えるのは SwarmLookSystem なので、体がプレイヤーを向く)。
	/// ・急降下は、始点(今の位置)・終点(狙いを終えた瞬間のプレイヤーの位置)・
	///   制御点(中間を m_arcHeight だけ持ち上げた点)の2次ベジェ曲線をなぞる。
	///   制御点を真ん中の真上に置くので、曲線は縦軸の放物線(y が水平距離の二次関数)になる。
	///   終点は固定するので、動いていれば避けられる。
	/// ・曲線は「曲線上を進む目標点」を追う形でなぞる(目標点の進む向き + ずれを詰める分)。
	///   速さは移動入力の長さで上げる(SwarmLeaderMoveSystem は 入力 × moveSpeed)。
	///   小隊長はリーダーの platoonSpeedScale 倍なので、それを超えると列が千切れる。
	/// ・終点に着いたら突進と同じく、同じ向きへ惰性で少し進んでから徘徊へ戻る
	///   (ボイドは地形をすり抜けるので、そのまま地面へ突っ込んでもよい)。
	/// ・プレイヤーが居なければ(急降下より前なら)何もせず徘徊へ戻る。
	/// </remarks>
	class SwarmBossDiveAttackState : public IState
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
			Rise,		// 離れる : プレイヤーから離れて高く上がる
			Hover,		// 狙う : ゆっくりプレイヤーへ向く
			Dive,		// 急降下 : 放物線をなぞってプレイヤーへ
			Recover,	// 余韻 : 惰性で進んでから徘徊へ
			End,		// 切り替え要求済み(次のフレームで抜ける)
		};

		// 次のフェーズへ(経過時間は0から数え直す)
		void ChangePhase(EPhase a_phase);

		// 急降下の曲線を今の位置からプレイヤーへ組む
		void BuildCurve(const Math::Vector3& a_start, const Math::Vector3& a_end);

		// 曲線上の点と接線(微分)。a_t は 0〜1
		Math::Vector3 CurvePos(float a_t) const;
		Math::Vector3 CurveTangent(float a_t) const;

		// 徘徊へ戻す(要求は一度だけ出す)
		void Finish(SwarmBossStateContext& a_context);

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		// 離れる
		float m_launchDistance  = 150.0f;	// プレイヤーからこの水平距離まで離れる
		float m_launchHeight    = 100.0f;	// プレイヤーからこの高さまで上がる
		float m_riseThrottle    = 1.0f;		// 離れるときの移動入力の強さ(0〜1)
		float m_arriveDistance  = 15.0f;	// 離れる先にこの距離まで近づいたら狙いへ
		float m_riseMaxTime     = 7.0f;		// 離れる最長時間(秒。着かなくてもその場から狙う)

		// 狙う
		float m_hoverTime       = 1.0f;		// 狙う長さ(秒)
		float m_hoverThrottle   = 0.2f;		// 狙う間の移動入力の強さ(0〜1。向きを合わせるのに少しは動かす)

		// 急降下
		float m_arcHeight       = 30.0f;	// 曲線の膨らみ(始点と終点の中間から持ち上げる高さ。0で直線)
		float m_diveSpeedScale  = 2.0f;		// 急降下の速さ(リーダーの移動速度に対する倍率)
		float m_followGain      = 4.0f;		// 曲線からのずれを詰める強さ(1/秒)
		float m_diveMaxTime     = 5.0f;		// 急降下の最長時間(秒)

		// 余韻
		float m_recoverTime     = 1.5f;		// 余韻の長さ(秒)
		float m_recoverThrottle = 0.6f;		// 余韻中の移動入力の強さ(0〜1)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		EPhase m_phase = EPhase::Rise;
		float m_phaseTime = 0.0f;				// 今のフェーズに入ってからの経過時間(秒)
		Math::Vector3 m_playerPos = {};			// 最後に見たプレイヤーの位置
		Math::Vector3 m_launchPos = {};			// 離れる先

		// 急降下の曲線(2次ベジェ : 始点・制御点・終点)と、曲線上の目標点の位置(0〜1)
		Math::Vector3 m_curveStart   = {};
		Math::Vector3 m_curveControl = {};
		Math::Vector3 m_curveEnd     = {};
		float m_curveT = 0.0f;

		Math::Vector3 m_moveDir = {};			// 最後に進んでいた向き(余韻で使う。単位ベクトル)
	};
}
