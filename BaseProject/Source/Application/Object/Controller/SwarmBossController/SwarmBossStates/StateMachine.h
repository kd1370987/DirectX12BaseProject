#pragma once

#include "IState.h"

namespace App::Object
{
	// ワームの行動、切り替え時は各ステートからチェンジ要求がきたら次のフレームで切り替える
	enum class ESwarmBossState
	{
		Idle,				// 地面から半身を出してる状態でプレイヤーを見つめつつ立っている状態。チンアナゴ的な
		RandomWalk,			// 徘徊行動 : 攻撃と攻撃の合間に移動。抽選が行われる ,地面の中と空中で分かれる
		UperAttack,			// 徘徊行動時に地面の中にいたら真下からプレイヤーに攻撃
		DiveAttack,			// プレイヤーから離れて二次関数的な曲線で現在位置からプレイヤーにだいぶする
		Charge,				// プレイヤーへ向かって一直線に突進する
	};

	class SwarmBossStateMachine
	{
	public:

		// ステートの登録。調整値を読み込みで受けるので、Archive より前(生成時)に呼ぶ
		void Init();

		// チェンジ要求があればここで切り替える(要求を出したフレームの次)
		void PreUpdate(SwarmBossStateContext& a_context);

		void Update(SwarmBossStateContext& a_context);

		void PostUpdate(SwarmBossStateContext& a_context);

		// 切り替え要求。実際に切り替わるのは次の PreUpdate
		void RequestChangeState(ESwarmBossState a_state);

		ESwarmBossState GetCurrentState() const { return m_currentState; }

		//------------------------------------------------------------------------------------------
		// シリアライズ / エディター : 登録済みの全ステートへ流す
		//------------------------------------------------------------------------------------------
		void Archive(Engine::Persistence::Archive& a_ar);
		void DrawInspector();

	private:

		// 要求されていたステートへ切り替える(Exit → Enter)
		void ChangeState(SwarmBossStateContext& a_context);

	private:

		// ステート
		std::unordered_map<ESwarmBossState, std::unique_ptr<IState>> m_upStates = {};

		IState* m_pCurrentState = nullptr;							// 現在のステートの実体(未開始なら null)
		ESwarmBossState m_currentState = ESwarmBossState::Idle;		// 現在のステート
		ESwarmBossState m_changeState  = ESwarmBossState::Idle;		// チェンジ要求
		bool m_isChangeRequested = false;							// 要求が出ているか
	};
}
