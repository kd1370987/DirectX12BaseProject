#pragma once

#include "../IState.h"

namespace App::Object
{
	/// <summary>
	/// 徘徊 : 生成位置のまわりでランダムに目標地点を選び、そこへ向かう
	/// </summary>
	/// <remarks>
	/// コントローラーはプレイヤーのキーボード/マウスと同じ立場で、作るのは移動入力だけ。
	/// 入力を速度へ変えるのは SwarmLeaderMoveSystem、向きを変えるのは SwarmLookSystem。
	///
	/// 目標地点は「着いたら」か「時間が来たら」選び直す。時間切れも見るのは、
	/// 障害物などで着けないまま止まってしまわないようにするため
	/// </remarks>
	class SwarmBossRandomWalkState : public IState
	{
	public:
		void Enter(SwarmBossStateContext& a_context) override;
		void Update(SwarmBossStateContext& a_context) override;
		void Exit(SwarmBossStateContext& a_context) override;

		void Archive(Engine::Persistence::Archive& a_ar) override;
		void DrawInspector() override;

	private:
		// 次の目標地点を抽選する(生成位置を中心にした範囲の中)
		void PickWanderTarget(const Math::Vector3& a_center);

	private:
		//------------------------------------------------------------------------------------------
		// 調整値
		//------------------------------------------------------------------------------------------
		float m_wanderRadius   = 60.0f;		// 生成位置からこの半径内で目標地点を選ぶ(水平)
		float m_wanderHeight   = 20.0f;		// 高さの振れ幅(生成位置から ±m)
		float m_wanderInterval = 6.0f;		// 目標地点を選び直す間隔(秒)
		float m_arriveDistance = 8.0f;		// この距離まで近づいたら次の目標地点へ
		float m_throttle       = 1.0f;		// 移動入力の強さ(0〜1)

		//------------------------------------------------------------------------------------------
		// 実行中の状態(保存しない)
		//------------------------------------------------------------------------------------------
		Math::Vector3 m_targetPos = {};		// リーダーのターゲット位置(ワールド)
		float m_wanderTimer = 0.0f;			// 次に選び直すまでの残り時間(秒)
	};
}
