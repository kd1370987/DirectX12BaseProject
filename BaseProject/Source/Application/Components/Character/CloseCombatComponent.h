#pragma once

#include "Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// CloseCombatComponent
//
// 近距離型の敵の「撃つ / 動き直す」のリズムを持つ。CloseCombatIntentSystem が進める。
//
// 遠距離型(Enemy_01)は間合いを保ったまま撃ち続けるが、近距離型はそれだと
// ただの棒立ちになる。足を止めて撃つ時間と、撃たずに位置を変える時間を
// 交互に持たせて「詰めて撃つ → 動く → また撃つ」の形にする。
//
// ・攻撃圏(TargetEntityComponent.isInAttackRange)の内側に入ってから回り出す。
//   見つけて詰めている途中は EnemyMoveIntentSystem の追従に任せ、ここは何もしない。
// ・撃つ相の間は移動入力を 0 にする。FSM の canMove を落とす手もあるが、
//   それだと状態を1つ足すことになり、他の敵の遷移まで見直しが要る。
//   移動入力を止めるだけなら FSM は Enemy_01 と同じものを使い回せる。
// ・動く相では撃たない。撃ちながら動くと足を止める意味が無くなる。
//==========================================================================================
struct CloseCombatComponent
{
	// ---- 設定(保存される) ----
	float fireTime     = 2.0f;	// 足を止めて撃ち続ける時間(秒)
	float moveTime     = 1.0f;	// 撃たずに動き直す時間(秒)
	float moveThrottle = 1.0f;	// 動き直すときのスロットル(0..1)

	// 動く向きのうち、横へ回り込む割合(0..1)。残りが間合いの詰め/離しに回る。
	// 1.0 にすると真横にしか動かないので、間合いがずれたまま戻らなくなる
	float strafeRatio  = 0.75f;

	// 動き直しで保ちたい間合い(m)。これより近ければ下がり、遠ければ寄る。
	// 攻撃圏(attackDistance)より内側にしておくこと。外に置くと
	// 動くたびに攻撃圏から出て、撃つ相に入った瞬間に撃てない
	float keepDistance = 18.0f;

	// ---- 状態(保存しない) ----
	bool  isFirePhase = true;	// 今が撃つ相か(攻撃圏へ入ったら撃つ相から始める)
	float timer       = 0.0f;	// 現在の相の経過時間(秒)
	float sideSign    = 1.0f;	// 横へ回る向き(+1 / -1)。動く相へ入るたびに選び直す
};

template<>
struct Engine::ECS::ComponentTraits<CloseCombatComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CloseCombatComponent& _comp = Engine::Editor::GetValue<CloseCombatComponent>(a_pData);
		a_ar.Field("fireTime", _comp.fireTime);
		a_ar.Field("moveTime", _comp.moveTime);
		a_ar.Field("moveThrottle", _comp.moveThrottle);
		a_ar.Field("strafeRatio", _comp.strafeRatio);
		a_ar.Field("keepDistance", _comp.keepDistance);
	}

	static void Edit(CompEditContext& a_context)
	{
		CloseCombatComponent& _comp = Engine::Editor::GetValue<CloseCombatComponent>(a_context.pData);
		ImGui::DragFloat("FireTime", &_comp.fireTime, 0.1f, 0.0f);
		ImGui::DragFloat("MoveTime", &_comp.moveTime, 0.1f, 0.0f);
		ImGui::DragFloat("MoveThrottle", &_comp.moveThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("StrafeRatio", &_comp.strafeRatio, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("KeepDistance", &_comp.keepDistance, 0.1f, 0.0f);

		ImGui::Separator();
		ImGui::Text("Phase : %s", _comp.isFirePhase ? "Fire" : "Move");
		ImGui::Text("Timer : %.2f", _comp.timer);
	}
};
