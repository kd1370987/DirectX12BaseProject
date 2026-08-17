#pragma once

//==========================================================================================
// MissileLockComponent
//
// プレイヤーに付ける「ミサイルの一斉射」の置き場。中身を作るのは MissileSalvoSystem。
//
//   ミサイルキーを押している間 … コンバットレティクル(CombatReticleHUD)の円に入った敵を
//                                 locks[] へ溜める(1体につき1ロック)
//   離した瞬間                  … missileCount 発を launchInterval 間隔で撃ち出す。
//                                 ロックした敵へ順番に割り振り(敵が足りなければ使い回す)、
//                                 弾はコーン状に散らしてから誘導で寄せる
//   撃ち終わり                  … cooldown 秒たつまで次の収集を始めない
//
// ・判定円の出どころは CombatReticleHUD。UI が毎フレーム reticleCenter / hudReticleRadius を
//   書き、システムはそれを使う。UI が無ければ画面中央 × 保存値 reticleRadius。
//   ロックオン(LockOnTargetComponent)は内側の AimReticleHUD が基準で、こちらとは別枠。
// ・弾のプレハブ・弾速・銃口ノードは、ミサイルポッド側の GunStateComponent を使う
//   (アタッチメントスロットの missile が指すエンティティ)。ここには持たない。
// ・固定長配列なのは ECS コンポーネントに std::vector を持たせないため
//   (アーキタイプ移動やプレハブ経路で memcpy される)。
//==========================================================================================
struct MissileLockComponent
{
	// 同時に扱えるロック/発射数の上限。missileCount はこれ以下に丸められる
	static constexpr int MISSILE_MAX = 16;

	// ---- 設定(保存される) ----
	int   missileCount   = 6;		// 1回の一斉射で撃つ数
	float cooldown       = 1.0f;	// 撃ち始めてから次に収集を始められるまで(秒)
	float launchInterval = 0.06f;	// 1発ごとの間隔(秒)。0 なら全弾同時に出る
	float spreadAngle    = 25.0f;	// 射出時の散らし角(度)。狙う向きを軸にしたコーンの半頂角
	float maxDistance    = 300.0f;	// ロックできる距離(m)。0 以下なら距離では切らない
	float targetOffsetY  = 0.0f;	// 敵の原点から上へずらす量(m)。原点が足元のモデル用
	float reticleRadius  = 200.0f;	// CombatReticleHUD が居ないときの判定半径(px)
	float reticleScale   = 1.0f;	// UI から来た半径に掛ける倍率(見た目より狭く/広く取る用)
	bool  requireLock    = false;	// true : 1つもロックできていなければ撃たない

	// ---- レティクル(ランタイム。CombatReticleHUD が書く) ----
	// 保存値(reticleRadius)は上書きしない。実行中に書き換えると、
	// エディターで見ている設定値が UI の値に置き換わってしまうため。
	DirectX::XMFLOAT2 reticleCenter    = { 0.0f, 0.0f };	// 判定の中心(px, 左上原点)
	float             hudReticleRadius = 0.0f;				// UI が出している半径(px)
	bool              isReticleFromHUD = false;				// UI から届いているか

	// 実際に使う判定の半径
	float GetActiveReticleRadius() const
	{
		return (isReticleFromHUD ? hudReticleRadius : reticleRadius) * reticleScale;
	}

	// 実際に撃つ数(上限で丸めたもの)
	int GetActiveMissileCount() const
	{
		return std::clamp(missileCount, 0, MISSILE_MAX);
	}

	// ---- 収集結果(ランタイム。保存しない) ----
	Engine::ECS::Entity locks[MISSILE_MAX]         = {};	// ロックした敵(重複なし)
	DirectX::XMFLOAT2   lockScreenPos[MISSILE_MAX] = {};	// その敵のスクリーン座標(px)。HUD 用
	int                 lockCount                  = 0;

	bool  isCharging    = false;	// 収集中(キーを押している)か
	bool  wasHold       = false;	// 立ち下がり(離した瞬間)の検出用
	float cooldownTimer = 0.0f;		// 残りクールダウン(秒)

	// ---- 発射待ち(ランタイム。保存しない) ----
	Engine::ECS::Entity fireTargets[MISSILE_MAX] = {};	// 各弾が追う相手。INVALID なら直進
	int                 fireRemain               = 0;	// 残り発射数
	int                 fireTotal                = 0;	// この一斉射の総数(散らしの角度計算用)
	float               fireTimer                = 0.0f;// 次の1発までの残り時間(秒)

	bool IsFiring()  const { return fireRemain > 0; }
	bool IsCoolDown()const { return cooldownTimer > 0.0f; }
};

template<>
struct Engine::ECS::ComponentTraits<MissileLockComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		MissileLockComponent& _comp = Engine::Editor::GetValue<MissileLockComponent>(a_pData);
		a_ar.Field("missileCount",   _comp.missileCount);
		a_ar.Field("cooldown",       _comp.cooldown);
		a_ar.Field("launchInterval", _comp.launchInterval);
		a_ar.Field("spreadAngle",    _comp.spreadAngle);
		a_ar.Field("maxDistance",    _comp.maxDistance);
		a_ar.Field("targetOffsetY",  _comp.targetOffsetY);
		a_ar.Field("reticleRadius",  _comp.reticleRadius);
		a_ar.Field("reticleScale",   _comp.reticleScale);
		a_ar.Field("requireLock",    _comp.requireLock);
	}

	static void Edit(CompEditContext& a_context)
	{
		MissileLockComponent& _comp = Engine::Editor::GetValue<MissileLockComponent>(a_context.pData);

		ImGui::Text("Salvo");
		if (ImGui::DragInt("MissileCount", &_comp.missileCount, 1, 0, MissileLockComponent::MISSILE_MAX))
		{
			_comp.missileCount = std::clamp(_comp.missileCount, 0, MissileLockComponent::MISSILE_MAX);
		}
		ImGui::DragFloat("Cooldown", &_comp.cooldown, 0.05f, 0.0f, 60.0f, "%.2f s");
		ImGui::DragFloat("LaunchInterval", &_comp.launchInterval, 0.01f, 0.0f, 5.0f, "%.3f s");
		ImGui::DragFloat("SpreadAngle", &_comp.spreadAngle, 0.5f, 0.0f, 89.0f, "%.1f deg");

		ImGui::Separator();

		ImGui::Text("Lock");
		ImGui::DragFloat("MaxDistance", &_comp.maxDistance, 1.0f, 0.0f);
		ImGui::DragFloat("TargetOffsetY", &_comp.targetOffsetY, 0.01f);
		ImGui::DragFloat("ReticleRadius", &_comp.reticleRadius, 1.0f, 0.0f, 4096.0f);
		ImGui::DragFloat("ReticleScale", &_comp.reticleScale, 0.01f, 0.0f, 4.0f);
		ImGui::Checkbox("RequireLock", &_comp.requireLock);

		// 結果は毎フレーム上書きされるので表示のみ
		ImGui::Separator();
		ImGui::Text("ReticleFromHUD : %s", _comp.isReticleFromHUD ? "yes" : "no");
		if (_comp.isReticleFromHUD)
		{
			ImGui::Text("ReticleCenter  : %.0f, %.0f", _comp.reticleCenter.x, _comp.reticleCenter.y);
		}
		ImGui::Text("ActiveRadius   : %.0f px", _comp.GetActiveReticleRadius());
		ImGui::Text("Charging : %s  Locks : %d", _comp.isCharging ? "yes" : "no", _comp.lockCount);
		ImGui::Text("FireRemain : %d / %d", _comp.fireRemain, _comp.fireTotal);
		ImGui::Text("Cooldown : %.2f s", _comp.cooldownTimer);

		ImGui::TextDisabled("Bullet prefab / speed / muzzle : MissilePod's GunStateComponent");
	}
};
