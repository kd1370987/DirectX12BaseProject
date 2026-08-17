#pragma once

//==========================================================================================
// LockOnTargetComponent
//
// プレイヤーに付ける「画面に映っている敵」と「ロック中の敵」の置き場。
// 中身を作るのは LockOnTargetSystem(PostUpdate)。
//
//   targets[]    … 画面内に映っている敵。UI では黄色の枠
//   lockedEntity … そのうちレティクル(reticleCenter から reticleRadius px)の内側で
//                   いちばん中心に近いもの。UI では赤い枠で、プレイヤーが体を向ける相手
//
// ・スクリーン座標まで持たせているのは、HUD(TargetBoxHUD)が同じ射影をもう一度やらずに
//   済ませるため。UI とロック判定が別々に射影すると、条件のわずかな差で
//   「枠は出ているのにロックされない」といったズレが起きる。
// ・ロック結果を読むのは HUD と LockOnRotationSystem(体の向き)。
//==========================================================================================
struct LockOnTargetComponent
{
	// 同時に枠を出せる数。増やすときはここだけ変えればよい
	// (保存しないランタイム配列なので、変えても既存データには影響しない)
	static constexpr int TARGET_MAX = 32;

	// ---- 設定(保存される) ----
	// レティクル判定の既定値。ロック(赤枠)の判定にだけ使う。黄色い枠は画面内なら出る。
	// 画面に AimReticleHUD(内側のレティクル)が居る場合は、
	// そちらが毎フレーム下の reticleCenter / reticleRadius を上書きする。
	// UI と判定を別々に持つと「枠の内側なのにロックされない」ズレが起きるため、
	// 実際に描いている UI 側を基準にする。
	float reticleRadius = 160.0f;	// 判定の半径(px)
	float maxDistance   = 300.0f;	// ロック可能な距離(m)。0 以下なら距離で切らない
	float targetOffsetY = 0.0f;		// 敵の原点から上へずらす量(m)。原点が足元のモデルで胴体に枠を合わせる用

	// ---- レティクル(ランタイム。AimReticleHUD が書く) ----
	// 保存値(reticleRadius)は上書きしない。実行中に書き換えると、
	// エディターで見ている設定値が UI の値に置き換わってしまうため。
	DirectX::XMFLOAT2 reticleCenter    = { 0.0f, 0.0f };	// 判定の中心(px, 左上原点)
	float             hudReticleRadius = 0.0f;				// UI が出している判定半径(px)
	bool              isReticleFromHUD = false;				// UI から届いているか(false なら画面中央 × reticleRadius)

	// 実際に使う判定の半径
	float GetActiveReticleRadius() const
	{
		return isReticleFromHUD ? hudReticleRadius : reticleRadius;
	}

	// ---- 結果(ランタイム。保存しない) ----
	Engine::ECS::Entity targets[TARGET_MAX]   = {};	// 画面内に映っている敵
	DirectX::XMFLOAT2   screenPos[TARGET_MAX] = {};	// その敵のスクリーン座標(px, 左上原点)
	int                 targetCount           = 0;	// 有効な要素数

	Engine::ECS::Entity lockedEntity    = Engine::ECS::Limits::INVALID_ENTITY;	// ロック中の敵
	DirectX::XMFLOAT2   lockedScreenPos = { 0.0f, 0.0f };						// その敵のスクリーン座標(px)
	DirectX::XMFLOAT3   lockedPos       = { 0.0f, 0.0f, 0.0f };					// その敵のワールド座標(旋回用)

	bool IsLocked() const { return lockedEntity != Engine::ECS::Limits::INVALID_ENTITY; }
};

template<>
struct Engine::ECS::ComponentTraits<LockOnTargetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LockOnTargetComponent& _comp = Engine::Editor::GetValue<LockOnTargetComponent>(a_pData);
		a_ar.Field("reticleRadius", _comp.reticleRadius);
		a_ar.Field("maxDistance", _comp.maxDistance);
		a_ar.Field("targetOffsetY", _comp.targetOffsetY);
	}

	static void Edit(CompEditContext& a_context)
	{
		LockOnTargetComponent& _comp = Engine::Editor::GetValue<LockOnTargetComponent>(a_context.pData);

		ImGui::DragFloat("ReticleRadius", &_comp.reticleRadius, 1.0f, 0.0f, 4096.0f);
		ImGui::DragFloat("MaxDistance", &_comp.maxDistance, 1.0f, 0.0f);
		ImGui::DragFloat("TargetOffsetY", &_comp.targetOffsetY, 0.01f);

		// 結果は毎フレーム上書きされるので表示のみ
		ImGui::Separator();
		ImGui::Text("ReticleFromHUD : %s", _comp.isReticleFromHUD ? "yes" : "no");
		if (_comp.isReticleFromHUD)
		{
			ImGui::Text("ReticleCenter  : %.0f, %.0f", _comp.reticleCenter.x, _comp.reticleCenter.y);
		}
		ImGui::Text("ActiveRadius   : %.0f px", _comp.GetActiveReticleRadius());
		ImGui::Text("Targets : %d", _comp.targetCount);
		ImGui::Text("Locked  : %s", _comp.IsLocked() ? "yes" : "no");
		if (_comp.IsLocked())
		{
			ImGui::Text("LockedScreen : %.0f, %.0f", _comp.lockedScreenPos.x, _comp.lockedScreenPos.y);
			ImGui::Text("LockedPos    : %.2f, %.2f, %.2f",
				_comp.lockedPos.x, _comp.lockedPos.y, _comp.lockedPos.z);
		}
	}
};
