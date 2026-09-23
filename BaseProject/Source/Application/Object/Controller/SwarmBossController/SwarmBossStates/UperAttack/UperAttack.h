#pragma once

#include "../IState.h"

namespace App::Object
{
	/// <summary>
	/// プレイヤーの真下まで移動して真上に向かって飛び出す
	/// </remarks>
	class SwarmBossUperAttackState : public IState
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
			Windup,		// 溜め : 減速してプレイヤーへ向く
			Uper,		// 突進 : 上向き回転しながら上がる
			Recover,	// 余韻 : 惰性で進んでから徘徊へ
			End,		// 切り替え要求済み(次のフレームで抜ける)
		};

		// 徘徊へ戻す(要求は一度だけ出す)
		void Finish(SwarmBossStateContext& a_context);

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		float m_windupTime = 1.0f;	// 溜めの長さ(秒)
		float m_windupThrottle = 0.3f;	// 溜め中の移動入力の強さ(0〜1。向きを合わせるのに少しは動かす)
		float m_chargeSpeedScale = 1.5f;	// 突進の速さ(リーダーの移動速度に対する倍率)
		float m_homingTurnSpeed = 0.5f;	// 突進中に曲がれる速さ(ラジアン/秒。0で直進)
		float m_maxChargeTime = 4.0f;	// 突進の最長時間(秒。外れても止まるように)
		float m_overshootDistance = 30.0f;	// プレイヤーをこの距離だけ通り過ぎたら突進をやめる
		float m_recoverTime = 1.5f;	// 余韻の長さ(秒)
		float m_recoverThrottle = 0.6f;	// 余韻中の移動入力の強さ(0〜1)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		EPhase m_phase = EPhase::Windup;
		float m_phaseTime = 0.0f;				// 今のフェーズに入ってからの経過時間(秒)
		Math::Vector3 m_playerPos = {};			// 最後に見たプレイヤーの位置(表示用)
	};
}
