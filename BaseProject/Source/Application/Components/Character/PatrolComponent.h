#pragma once

//==========================================================================================
// PatrolComponent
//
// 敵の移動 AI 用の設定と徘徊状態を保持する。EnemyMoveIntentSystem が参照/更新する。
//
// ・徘徊(パトロール): プレイヤー未発見時、一定間隔でランダムな水平方向へ向かって歩く。
// ・追跡(チェイス)  : プレイヤー発見時、プレイヤー方向へ歩く。stopDistance まで近づくと止まる。
//
// 実際の速度は throttle(0..1) × maxSpeed。さらに FSM 側の moveSpeedScale / canMove で
// ActionBehaviorSystem がゲート・スケールする(=状態が最終的な行動を決める)。
//==========================================================================================
struct PatrolComponent
{
	// ---- 設定(保存される) ----
	float maxSpeed         = 4.0f;	// throttle=1.0 のときの速度(units/sec)
	float patrolThrottle   = 0.4f;	// 徘徊時のスロットル(0..1)
	float chaseThrottle    = 1.0f;	// 追跡時のスロットル(0..1)
	float retargetInterval = 2.5f;	// 徘徊方向を変える間隔(秒)
	float stopDistance     = 2.0f;	// 追跡時、これ以下まで近づいたら止まる(攻撃間合い)

	// ---- 徘徊状態(ランタイム) ----
	DirectX::XMFLOAT3 wanderDir = { 0.0f, 0.0f, 1.0f };	// 現在の徘徊方向(世界/水平/単位)
	float             wanderTimer = 0.0f;				// 次に方向転換するまでの残り時間
};

template<>
struct Engine::ECS::ComponentTraits<PatrolComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PatrolComponent& _comp = Engine::Editor::GetValue<PatrolComponent>(a_pData);
		a_ar.Field("maxSpeed", _comp.maxSpeed);
		a_ar.Field("patrolThrottle", _comp.patrolThrottle);
		a_ar.Field("chaseThrottle", _comp.chaseThrottle);
		a_ar.Field("retargetInterval", _comp.retargetInterval);
		a_ar.Field("stopDistance", _comp.stopDistance);
	}

	static void Edit(CompEditContext& a_context)
	{
		PatrolComponent& _comp = Engine::Editor::GetValue<PatrolComponent>(a_context.pData);
		ImGui::DragFloat("MaxSpeed", &_comp.maxSpeed, 0.1f, 0.0f);
		ImGui::DragFloat("PatrolThrottle", &_comp.patrolThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("ChaseThrottle", &_comp.chaseThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("RetargetInterval", &_comp.retargetInterval, 0.1f, 0.0f);
		ImGui::DragFloat("StopDistance", &_comp.stopDistance, 0.1f, 0.0f);

		ImGui::Separator();
		ImGui::Text("WanderDir : %.2f, %.2f, %.2f", _comp.wanderDir.x, _comp.wanderDir.y, _comp.wanderDir.z);
		ImGui::Text("WanderTimer : %.2f", _comp.wanderTimer);
	}
};
