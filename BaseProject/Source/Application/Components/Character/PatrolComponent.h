#pragma once

//==========================================================================================
// PatrolComponent
//
// 敵の移動 AI 用の設定と徘徊状態を保持する。EnemyMoveIntentSystem が参照/更新する。
//
// ・徘徊(パトロール): プレイヤー未発見時、「ランダムな水平方向へ歩く → 立ち止まって
//     少し首を振る」を交互に繰り返す。立ち止まるたびに次の方向を選び直す。
// ・追跡(チェイス)  : プレイヤー発見時、プレイヤー方向へ歩く。stopDistance まで近づくと止まる。
// ・見失い(ロストターゲット):
//     発見済みのプレイヤーを見失ったら、最後に見た地点まで移動し、着いたら周囲を見渡す。
//     見渡している間に再発見できれば追跡へ戻り、できなければ徘徊へ戻る。
//
// 実際の速度は throttle(0..1) × maxSpeed。さらに FSM 側の moveSpeedScale / canMove で
// ActionBehaviorSystem がゲート・スケールする(=状態が最終的な行動を決める)。
//==========================================================================================

// 徘徊のフェーズ
enum class EPatrolPhase : int
{
	Move = 0,	// 選んだ方向へ歩いている
	Pause,		// 立ち止まって首を振っている
};

// 見失い探索のフェーズ
enum class ELostPhase : int
{
	None = 0,		// 探索していない(徘徊 or 追跡中)
	MoveTo,			// 最後に見た地点へ向かっている
	LookAround,		// 地点に着いて周囲を見渡している
};

struct PatrolComponent
{
	// ---- 設定(保存される) ----
	float maxSpeed         = 4.0f;	// throttle=1.0 のときの速度(units/sec)
	float patrolThrottle   = 0.4f;	// 徘徊時のスロットル(0..1)
	float chaseThrottle    = 1.0f;	// 追跡時のスロットル(0..1)
	float retargetInterval = 2.5f;	// 徘徊で1つの方向へ歩き続ける時間(秒)

	// ---- 追跡時に保つ間合い(保存される) ----
	// 遠隔攻撃なので「近づきすぎず離れすぎず」を保つ。
	//   stopDistance + keepMargin より遠い … 詰める
	//   stopDistance - keepMargin より近い … 下がる
	//   その間                             … 止まる(デッドバンド)
	// ※ 攻撃可能距離(TargetEntityComponent.attackDistance)より内側で止まること。
	//    外で止まると永久に攻撃圏へ入れないので、EnemyMoveIntentSystem 側でも
	//    attackDistance - keepMargin を上限として寄せている。
	//    ちょうど境界上で止まる設定にすると、攻撃可能/不能を往復してしまう。
	float stopDistance  = 15.0f;	// 保ちたい間合い(この距離で止まる)
	float keepMargin    = 2.0f;		// 間合いの許容幅(±)。0 にすると境界で震える
	float backThrottle  = 0.6f;		// 近づかれすぎて下がるときのスロットル(0..1)

	// ---- 徘徊で立ち止まったときの設定(保存される) ----
	float patrolPauseTime   = 2.0f;		// 立ち止まって首を振る時間(秒)
	float patrolLookYawDeg  = 45.0f;	// そのときの首振り幅(止まった向きから左右へ何度)

	// ---- 見失い探索の設定(保存される) ----
	float lostThrottle       = 0.8f;	// 最後に見た地点へ向かうときのスロットル(0..1)
	float lostArriveDistance = 1.5f;	// この距離まで近づいたら「到着」とみなす
	float lostMoveTimeout    = 8.0f;	// 到着できないまま諦めるまでの時間(秒)
	float lookAroundTime     = 3.0f;	// 到着後に周囲を見渡す時間(秒)
	float lookAroundYawDeg   = 100.0f;	// 見渡す振れ幅(到着時の向きから左右へ何度)
	float lookAroundSpeedDeg = 120.0f;	// 見渡すときの旋回速度(度/秒)

	// ---- 徘徊状態(ランタイム) ----
	DirectX::XMFLOAT3 wanderDir = { 0.0f, 0.0f, 1.0f };			// 現在の徘徊方向(世界/水平/単位)
	float             wanderTimer = 0.0f;						// 現フェーズの残り時間(秒)
	EPatrolPhase      patrolPhase = EPatrolPhase::Pause;		// 現在のフェーズ(初回は方向選びから始める)
	float             patrolLookYaw = 0.0f;						// 首振りの基準ヨー(止まった向き/ラジアン)

	// ---- 見失い探索の状態(ランタイム) ----
	ELostPhase        lostPhase     = ELostPhase::None;			// 現在のフェーズ
	DirectX::XMFLOAT3 lastSeenPos   = { 0.0f, 0.0f, 0.0f };		// 最後にプレイヤーを見た位置
	float             lostTimer     = 0.0f;						// 現フェーズの経過時間(秒)
	float             lookAroundYaw = 0.0f;						// 見渡しの基準ヨー(到着時の向き/ラジアン)
	bool              wasFind       = false;					// 前フレームの isFind(見失った瞬間の検出用)
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
		a_ar.Field("keepMargin", _comp.keepMargin);
		a_ar.Field("backThrottle", _comp.backThrottle);

		// 徘徊で立ち止まったとき。旧データにキーが無い場合は既定値のまま読み飛ばされる。
		a_ar.Field("patrolPauseTime", _comp.patrolPauseTime);
		a_ar.Field("patrolLookYawDeg", _comp.patrolLookYawDeg);

		// 見失い探索。旧データにキーが無い場合は既定値のまま読み飛ばされる。
		a_ar.Field("lostThrottle", _comp.lostThrottle);
		a_ar.Field("lostArriveDistance", _comp.lostArriveDistance);
		a_ar.Field("lostMoveTimeout", _comp.lostMoveTimeout);
		a_ar.Field("lookAroundTime", _comp.lookAroundTime);
		a_ar.Field("lookAroundYawDeg", _comp.lookAroundYawDeg);
		a_ar.Field("lookAroundSpeedDeg", _comp.lookAroundSpeedDeg);
	}

	static void Edit(CompEditContext& a_context)
	{
		PatrolComponent& _comp = Engine::Editor::GetValue<PatrolComponent>(a_context.pData);
		ImGui::DragFloat("MaxSpeed", &_comp.maxSpeed, 0.1f, 0.0f);
		ImGui::DragFloat("PatrolThrottle", &_comp.patrolThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("ChaseThrottle", &_comp.chaseThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("RetargetInterval", &_comp.retargetInterval, 0.1f, 0.0f);
		ImGui::DragFloat("StopDistance", &_comp.stopDistance, 0.1f, 0.0f);
		ImGui::DragFloat("KeepMargin", &_comp.keepMargin, 0.1f, 0.0f);
		ImGui::DragFloat("BackThrottle", &_comp.backThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("PatrolPauseTime", &_comp.patrolPauseTime, 0.1f, 0.0f);
		ImGui::DragFloat("PatrolLookYawDeg", &_comp.patrolLookYawDeg, 1.0f, 0.0f, 180.0f);

		ImGui::SeparatorText("LostTarget");
		ImGui::DragFloat("LostThrottle", &_comp.lostThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("LostArriveDistance", &_comp.lostArriveDistance, 0.1f, 0.0f);
		ImGui::DragFloat("LostMoveTimeout", &_comp.lostMoveTimeout, 0.1f, 0.0f);
		ImGui::DragFloat("LookAroundTime", &_comp.lookAroundTime, 0.1f, 0.0f);
		ImGui::DragFloat("LookAroundYawDeg", &_comp.lookAroundYawDeg, 1.0f, 0.0f, 180.0f);
		ImGui::DragFloat("LookAroundSpeedDeg", &_comp.lookAroundSpeedDeg, 1.0f, 0.0f);

		ImGui::Separator();
		static const char* _patrolPhaseName[] = { "Move", "Pause" };
		ImGui::Text("PatrolPhase : %s", _patrolPhaseName[static_cast<int>(_comp.patrolPhase)]);
		ImGui::Text("WanderDir : %.2f, %.2f, %.2f", _comp.wanderDir.x, _comp.wanderDir.y, _comp.wanderDir.z);
		ImGui::Text("WanderTimer : %.2f", _comp.wanderTimer);

		static const char* _lostPhaseName[] = { "None", "MoveTo", "LookAround" };
		ImGui::Text("LostPhase : %s", _lostPhaseName[static_cast<int>(_comp.lostPhase)]);
		ImGui::Text("LastSeenPos : %.2f, %.2f, %.2f", _comp.lastSeenPos.x, _comp.lastSeenPos.y, _comp.lastSeenPos.z);
		ImGui::Text("LostTimer : %.2f", _comp.lostTimer);
	}
};
