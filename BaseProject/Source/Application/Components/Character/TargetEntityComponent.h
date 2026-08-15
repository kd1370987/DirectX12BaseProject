#pragma once

//==========================================================================================
// TargetEntityComponent
//
// 敵が狙うターゲット(プレイヤー)と、その索敵結果を持つ。SearchPlayerSystem が更新する。
//
// 索敵は距離だけで行う(視界コーン/遮蔽判定は廃止)。距離は 2 段階に分かれている。
//
//   発見距離(detectDistance)   … ここまで近づかれたら戦闘モード(isFind)へ入り、追従を始める
//   攻撃可能距離(attackDistance)… ここまで詰めたら攻撃してよい(isInAttackRange)
//
// つまり「発見 → 追いかける → 攻撃圏に入ったら撃つ」の三段構え。
// attackDistance < detectDistance にしておくこと(逆にすると発見と同時に撃ち始める)。
//
// どちらの距離も「入る距離」と「抜ける距離」を分けている。同じ値にすると、
// 境界上でプレイヤーが少し動くだけで入る/抜けるを往復(チャタリング)してしまうため。
//==========================================================================================
struct TargetEntityComponent
{
	Engine::GUID targetGUID = Engine::DefaultGUID;
	Engine::ECS::Entity targetEntity = Engine::ECS::Limits::INVALID_ENTITY;

	// ---- 発見距離(保存される) ----
	float detectDistance     = 30.0f;	// この距離まで近づかれたら戦闘モードに入る(= 追従開始)
	float detectExitDistance = 40.0f;	// この距離より離れられたら戦闘モードを抜ける(発見距離以上にすること)

	// ---- 攻撃可能距離(保存される) ----
	float attackDistance     = 20.0f;	// この距離まで詰めたら攻撃可能(発見距離より内側にすること)
	float attackExitDistance = 26.0f;	// この距離より離れられたら攻撃をやめて追従へ戻る(攻撃可能距離以上にすること)

	// ---- 索敵結果(ランタイム) ----
	bool isFind = false;			// 戦闘モード中か(発見距離の内側)
	bool isInAttackRange = false;	// 攻撃可能圏の内側か
	float distance = 0.0f;			// ターゲットまでの距離(3D・高低差込み)
};

template<>
struct Engine::ECS::ComponentTraits<TargetEntityComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TargetEntityComponent& _comp = Engine::Editor::GetValue<TargetEntityComponent>(a_pData);
		a_ar.Field("targetGUID",_comp.targetGUID);

		// 索敵距離。旧データにキーが無い場合は既定値のまま読み飛ばされる。
		a_ar.Field("detectDistance", _comp.detectDistance);
		a_ar.Field("detectExitDistance", _comp.detectExitDistance);
		a_ar.Field("attackDistance", _comp.attackDistance);
		a_ar.Field("attackExitDistance", _comp.attackExitDistance);
	}

	static void Edit(CompEditContext& a_context)
	{
		TargetEntityComponent& _comp = Engine::Editor::GetValue<TargetEntityComponent>(a_context.pData);
		ImGui::SeparatorText("Detect");
		ImGui::DragFloat("DetectDistance", &_comp.detectDistance, 0.1f, 0.0f);
		ImGui::DragFloat("DetectExitDistance", &_comp.detectExitDistance, 0.1f, 0.0f);

		ImGui::SeparatorText("Attack");
		ImGui::DragFloat("AttackDistance", &_comp.attackDistance, 0.1f, 0.0f);
		ImGui::DragFloat("AttackExitDistance", &_comp.attackExitDistance, 0.1f, 0.0f);

		ImGui::Separator();
		bool _isFind = _comp.isFind;
		ImGui::Checkbox("IsFind",&_isFind);
		bool _isInAttackRange = _comp.isInAttackRange;
		ImGui::Checkbox("IsInAttackRange",&_isInAttackRange);
		ImGui::Text("Distance : %f",_comp.distance);

	}
};
