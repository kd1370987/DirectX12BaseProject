#pragma once

//==========================================================================================
// BossComponent
//
// 人型ボス(アーマードコア / オメガフェニックス型)の戦闘設定と機動状態を持つ。
// 中身を進めるのは BossCombatIntentSystem(PreUpdate)と BossMissileSalvoSystem(PostUpdate)。
//
// ・ザコ敵(PatrolComponent)との違い
//     ザコは「徘徊 → 発見 → 追跡 → 攻撃」を距離だけで自動的に始める。
//     ボスは徘徊せず、シーケンス(SceneSequence)から戦闘開始命令が届くまで待機する。
//     戦闘に入ってからは地面を歩かず、ブーストで空中を高速に動き回りながら撃ち合う。
//
// ・動かし方はプレイヤーと同じ部品を使い回している
//     LookAngleComponent  … 「どこを向いているか」。ボスAIが相手の方向へ毎フレーム寄せる。
//                           機体の向きは RotationSystem、上体の狙いは AdditivePoseSystem、
//                           ブーストの向きは RobotBoostSystem がこの角度を読む。
//     MoveIntentComponent … 視点基準の移動入力(x=横 / y=上下 / z=前後)。プレイヤーの
//                           入力とまったく同じ意味なので CharacterMovementSystem が使える。
//     BoostComponent      … ブースト入力。RobotBoostSystem がそのまま推力に変える。
//     ActionIntent / AimTargetPos
//                         … 銃の発射入力と狙点。AttachmentDispatchSystem が武器の子
//                           エンティティへ配信し、GunShootSystem が撃つ。
//   つまりボス用に増やしたのは「入力を作る側」だけで、動かす側は全部既存のもの。
//
// ・撃つ相手は TargetEntityComponent(SearchPlayerSystem が解決する)。
//   戦闘の開始/終了は距離ではなく命令で決めるので、発見距離は広めに取っておくこと
//   (弾の誘導先を引く GunShootSystem が isFind を見るため)。
//==========================================================================================

// ボスの機動フェーズ(表示用。判断は毎フレーム距離から決め直す)
enum class EBossManeuver : int
{
	Wait = 0,	// 戦闘開始命令を待っている
	Approach,	// 間合いより遠い : 詰める
	Keep,		// 間合いの内   : 横に流しながら撃ち合う
	Back,		// 近すぎる     : 下がる
	Hold,		// 横の切り返しで足を止めている(撃たれてもよい隙)
};

//==========================================================================================
// ボスの行動パターン
//
// 一定時間ごとに重み付きの抽選で選び直す「今回はどう戦うか」。
// パターンが決めるのは “どこに居たいか” だけで、そこへ行く手順(間合いを保つ・横へ流す・
// 高度を合わせる)はパターンによらず共通。位置取りの目標を差し替えるだけで
// 「詰めてくる」「上を取る」「下から来る」が出せる。
//
// 同じパターンが連続しないように選ぶので、待ち構えていると読みが外れる。
//==========================================================================================
enum class EBossPattern : int
{
	Standoff = 0,	// 既定の間合いで正面から撃ち合う
	Rush,			// 懐まで一気に詰める
	HighGround,		// 大きく上を取って撃ち下ろす
	LowGround,		// 低く潜り込んで撃ち上げる
	Orbit,			// 間合いを保ったまま同じ向きへ回り込む
	Retreat,		// 大きく離れてミサイル主体で削る

	Max				// 抽選で回すための番兵(パターン数)
};

struct BossComponent
{
	// ---- 戦闘開始 ----
	// isCombatStarted はシーケンスからの命令で立つランタイム値。保存しない
	// (保存してしまうと、シーンを読み直しただけで戦闘が始まってしまう)。
	bool isCombatStarted = false;
	bool startOnSpawn    = false;	// 命令を待たずに開始する(単体で動きを見たいとき用。保存する)

	// ---- 間合い(保存される) ----
	// 保ちたい距離を1点ではなく幅で持つ。境界ちょうどを狙うと詰める/下がるを
	// 毎フレーム往復してしまうため(ザコの keepMargin と同じ考え)。
	// ここは Standoff(既定)の値で、行動パターンごとに差し替わる。
	float keepDistance = 45.0f;		// 保ちたい間合い(m)
	float keepMargin   = 12.0f;		// その許容幅(±m)。この中では前後に動かない
	float keepHeight   = 10.0f;		// プレイヤーからどれだけ上に居たいか(m)
	float heightMargin = 3.0f;		// 高さの許容幅(±m)

	// ---- 行動パターン(保存される) ----
	// 一定時間ごとに抽選し直す。パターンは位置取りの目標を差し替えるだけで、
	// そこへ行く手順は共通(間合いを保つ / 横へ流す / 高度を合わせる)。
	float patternDuration     = 4.0f;	// 1つのパターンを続ける時間(秒)
	float patternDurationRand = 1.5f;	// その揺らぎ(±秒)

	float rushDistance     = 12.0f;		// Rush       : 詰める間合い(m)
	float rushHeight       = 3.0f;		// Rush       : そのときの高さ(相手から±m)
	float highGroundHeight = 35.0f;		// HighGround : 相手からどれだけ上へ(m)
	float lowGroundHeight  = -8.0f;		// LowGround  : 相手からどれだけ下へ(m。負で下)
	float retreatDistance  = 90.0f;		// Retreat    : 離れる間合い(m)
	float orbitStrafeScale = 1.2f;		// Orbit      : 横移動の強さ倍率

	// 抽選の重み。0 にするとそのパターンは出なくなる。
	// 「1つだけ 0 以外」にすれば、そのパターンだけで戦わせて動きを確認できる。
	float weightStandoff   = 3.0f;
	float weightRush       = 2.0f;
	float weightHighGround = 2.0f;
	float weightLowGround  = 1.5f;
	float weightOrbit      = 2.0f;
	float weightRetreat    = 1.0f;

	// ---- 旋回(保存される) ----
	float turnSpeedDeg  = 220.0f;	// 視点の水平旋回速度(度/秒)
	float pitchSpeedDeg = 160.0f;	// 視点の上下旋回速度(度/秒)
	float maxPitchDeg   = 60.0f;	// 見上げ/見下ろしの限界(±度)

	// ---- 機動(保存される) ----
	// スロットルは MoveIntent の大きさ(0..1)。実速度は MovementComponent.moveSpeed と
	// BoostComponent.boostPower 側で決まる。
	float strafeInterval     = 1.4f;	// 横移動の向きを切り替える間隔(秒)
	float strafeIntervalRand = 0.8f;	// その揺らぎ(±秒)。同じ周期で往復すると読まれてしまう
	float strafeThrottle     = 1.0f;	// 横移動のスロットル(0..1)

	// ---- 切り返しの静止(保存される) ----
	// 撃ち合いの最中、横に振る動きの折り返しで足を止める。
	// 常に横へ流れ続けていると狙いを置く先が定まらず、当てるのがほぼ運になってしまうため、
	// 「止まっているあいだは狙って当てられる」という隙をこちらから作る。
	// 詰め(Rush)と離脱(Retreat)では止まらない(移動そのものが目的の動きなので)。
	float strafeHoldChance   = 0.5f;	// 折り返しで止まる確率(0..1)。0 で止まらない
	float strafeHoldTimeMin  = 1.0f;	// 止まっている時間の下限(秒)
	float strafeHoldTimeMax  = 2.0f;	// 止まっている時間の上限(秒)
	float approachThrottle   = 1.0f;	// 詰めるときのスロットル(0..1)
	float backThrottle       = 1.0f;	// 下がるときのスロットル(0..1)
	float verticalThrottle   = 1.0f;	// 上下移動のスロットル(0..1)

	float boostFuelReserve   = 12.0f;	// 残量がこれを下回ったらブーストを休む(0 なら切れるまで吹かす)
	float dashInterval       = 2.2f;	// クイックブースト(単押し)の間隔(秒)
	float dashIntervalRand   = 1.0f;	// その揺らぎ(±秒)

	// ---- 銃(保存される) ----
	float gunRange     = 120.0f;	// この距離まで詰めたら撃つ(m)
	float gunConeDeg   = 35.0f;		// 正面この角度に相手が入っていたら撃つ(度)
	float gunBurstTime = 1.6f;		// 撃ち続ける時間(秒)
	float gunRestTime  = 0.9f;		// 撃つのを休む時間(秒)

	// ---- 狙い(保存される) ----
	float aimOffsetY   = 3.0f;		// 狙点を相手の原点から上へずらす量(m)。原点が足元のモデル用
	float aimLeadScale = 1.0f;		// 偏差撃ちの強さ。0 で置き撃ちなし、1 で弾速から求めた分だけ先を狙う

	// ---- ミサイル(保存される) ----
	float missileRange        = 160.0f;	// この距離まで詰めたら一斉射する(m)
	float missileInterval     = 6.0f;	// 一斉射の間隔(秒)
	float missileIntervalRand = 2.0f;	// その揺らぎ(±秒)

	// ---- ランタイム(保存しない) ----
	EBossManeuver maneuver = EBossManeuver::Wait;		// 今の機動フェーズ(表示用)
	EBossPattern  pattern  = EBossPattern::Standoff;	// 今の行動パターン
	float patternTimer     = 0.0f;		// 次にパターンを選び直すまでの残り時間(秒)
	float strafeSign       = 1.0f;		// 横移動の向き(+1 / -1)
	float strafeTimer      = 0.0f;		// 次に横移動を切り替えるまでの残り時間(秒)
	float strafeHoldTimer  = 0.0f;		// 足を止めている残り時間(秒)。0 なら動いている
	float dashTimer        = 0.0f;		// 次のクイックブーストまでの残り時間(秒)
	float gunTimer         = 0.0f;		// 撃つ/休むの残り時間(秒)
	bool  isGunActive      = false;		// 今は撃つ番か
	float missileTimer     = 0.0f;		// 次の一斉射までの残り時間(秒)
	bool  isMissileRequest = false;		// 一斉射の要求。BossMissileSalvoSystem が消費する
	float distance         = 0.0f;		// 相手までの距離(m。表示用)
};

template<>
struct Engine::ECS::ComponentTraits<BossComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BossComponent& _comp = Engine::Editor::GetValue<BossComponent>(a_pData);

		a_ar.Field("startOnSpawn", _comp.startOnSpawn);

		a_ar.Field("keepDistance", _comp.keepDistance);
		a_ar.Field("keepMargin", _comp.keepMargin);
		a_ar.Field("keepHeight", _comp.keepHeight);
		a_ar.Field("heightMargin", _comp.heightMargin);

		a_ar.Field("patternDuration", _comp.patternDuration);
		a_ar.Field("patternDurationRand", _comp.patternDurationRand);
		a_ar.Field("rushDistance", _comp.rushDistance);
		a_ar.Field("rushHeight", _comp.rushHeight);
		a_ar.Field("highGroundHeight", _comp.highGroundHeight);
		a_ar.Field("lowGroundHeight", _comp.lowGroundHeight);
		a_ar.Field("retreatDistance", _comp.retreatDistance);
		a_ar.Field("orbitStrafeScale", _comp.orbitStrafeScale);

		a_ar.Field("weightStandoff", _comp.weightStandoff);
		a_ar.Field("weightRush", _comp.weightRush);
		a_ar.Field("weightHighGround", _comp.weightHighGround);
		a_ar.Field("weightLowGround", _comp.weightLowGround);
		a_ar.Field("weightOrbit", _comp.weightOrbit);
		a_ar.Field("weightRetreat", _comp.weightRetreat);

		a_ar.Field("turnSpeedDeg", _comp.turnSpeedDeg);
		a_ar.Field("pitchSpeedDeg", _comp.pitchSpeedDeg);
		a_ar.Field("maxPitchDeg", _comp.maxPitchDeg);

		a_ar.Field("strafeInterval", _comp.strafeInterval);
		a_ar.Field("strafeIntervalRand", _comp.strafeIntervalRand);
		a_ar.Field("strafeThrottle", _comp.strafeThrottle);
		a_ar.Field("strafeHoldChance", _comp.strafeHoldChance);
		a_ar.Field("strafeHoldTimeMin", _comp.strafeHoldTimeMin);
		a_ar.Field("strafeHoldTimeMax", _comp.strafeHoldTimeMax);
		a_ar.Field("approachThrottle", _comp.approachThrottle);
		a_ar.Field("backThrottle", _comp.backThrottle);
		a_ar.Field("verticalThrottle", _comp.verticalThrottle);

		a_ar.Field("boostFuelReserve", _comp.boostFuelReserve);
		a_ar.Field("dashInterval", _comp.dashInterval);
		a_ar.Field("dashIntervalRand", _comp.dashIntervalRand);

		a_ar.Field("gunRange", _comp.gunRange);
		a_ar.Field("gunConeDeg", _comp.gunConeDeg);
		a_ar.Field("gunBurstTime", _comp.gunBurstTime);
		a_ar.Field("gunRestTime", _comp.gunRestTime);

		a_ar.Field("aimOffsetY", _comp.aimOffsetY);
		a_ar.Field("aimLeadScale", _comp.aimLeadScale);

		a_ar.Field("missileRange", _comp.missileRange);
		a_ar.Field("missileInterval", _comp.missileInterval);
		a_ar.Field("missileIntervalRand", _comp.missileIntervalRand);
	}

	static void Edit(CompEditContext& a_context)
	{
		BossComponent& _comp = Engine::Editor::GetValue<BossComponent>(a_context.pData);

		ImGui::SeparatorText("Combat Start");
		ImGui::Checkbox("StartOnSpawn", &_comp.startOnSpawn);
		ImGui::SameLine();
		ImGui::TextDisabled("(no order needed)");

		// 命令はランタイム値。動きを確かめたいときのためにエディターからも叩けるようにしておく
		ImGui::Text("CombatStarted : %s", _comp.isCombatStarted ? "yes" : "no");
		ImGui::SameLine();
		if (ImGui::SmallButton(_comp.isCombatStarted ? "Stop" : "Start"))
		{
			_comp.isCombatStarted = !_comp.isCombatStarted;
		}

		ImGui::SeparatorText("Range (Standoff)");
		ImGui::DragFloat("KeepDistance", &_comp.keepDistance, 0.5f, 0.0f);
		ImGui::DragFloat("KeepMargin", &_comp.keepMargin, 0.1f, 0.0f);
		ImGui::DragFloat("KeepHeight", &_comp.keepHeight, 0.1f);
		ImGui::DragFloat("HeightMargin", &_comp.heightMargin, 0.1f, 0.0f);

		ImGui::SeparatorText("Pattern");
		ImGui::DragFloat("PatternDuration", &_comp.patternDuration, 0.1f, 0.0f);
		ImGui::DragFloat("PatternDurationRand", &_comp.patternDurationRand, 0.1f, 0.0f);
		ImGui::DragFloat("RushDistance", &_comp.rushDistance, 0.5f, 0.0f);
		ImGui::DragFloat("RushHeight", &_comp.rushHeight, 0.1f);
		ImGui::DragFloat("HighGroundHeight", &_comp.highGroundHeight, 0.5f);
		ImGui::DragFloat("LowGroundHeight", &_comp.lowGroundHeight, 0.5f);
		ImGui::DragFloat("RetreatDistance", &_comp.retreatDistance, 0.5f, 0.0f);
		ImGui::DragFloat("OrbitStrafeScale", &_comp.orbitStrafeScale, 0.05f, 0.0f, 3.0f);

		ImGui::TextDisabled("Weight (0 = never picked)");
		ImGui::DragFloat("W:Standoff", &_comp.weightStandoff, 0.1f, 0.0f);
		ImGui::DragFloat("W:Rush", &_comp.weightRush, 0.1f, 0.0f);
		ImGui::DragFloat("W:HighGround", &_comp.weightHighGround, 0.1f, 0.0f);
		ImGui::DragFloat("W:LowGround", &_comp.weightLowGround, 0.1f, 0.0f);
		ImGui::DragFloat("W:Orbit", &_comp.weightOrbit, 0.1f, 0.0f);
		ImGui::DragFloat("W:Retreat", &_comp.weightRetreat, 0.1f, 0.0f);

		ImGui::SeparatorText("Turn");
		ImGui::DragFloat("TurnSpeedDeg", &_comp.turnSpeedDeg, 1.0f, 0.0f);
		ImGui::DragFloat("PitchSpeedDeg", &_comp.pitchSpeedDeg, 1.0f, 0.0f);
		ImGui::DragFloat("MaxPitchDeg", &_comp.maxPitchDeg, 1.0f, 0.0f, 89.0f);

		ImGui::SeparatorText("Maneuver");
		ImGui::DragFloat("StrafeInterval", &_comp.strafeInterval, 0.05f, 0.0f);
		ImGui::DragFloat("StrafeIntervalRand", &_comp.strafeIntervalRand, 0.05f, 0.0f);
		ImGui::DragFloat("StrafeThrottle", &_comp.strafeThrottle, 0.01f, 0.0f, 1.0f);

		ImGui::TextDisabled("Hold (pause at strafe turn-around)");
		ImGui::DragFloat("StrafeHoldChance", &_comp.strafeHoldChance, 0.01f, 0.0f, 1.0f);
		if (ImGui::DragFloat("StrafeHoldTimeMin", &_comp.strafeHoldTimeMin, 0.05f, 0.0f))
		{
			if (_comp.strafeHoldTimeMax < _comp.strafeHoldTimeMin) _comp.strafeHoldTimeMax = _comp.strafeHoldTimeMin;
		}
		if (ImGui::DragFloat("StrafeHoldTimeMax", &_comp.strafeHoldTimeMax, 0.05f, 0.0f))
		{
			if (_comp.strafeHoldTimeMax < _comp.strafeHoldTimeMin) _comp.strafeHoldTimeMin = _comp.strafeHoldTimeMax;
		}
		ImGui::DragFloat("ApproachThrottle", &_comp.approachThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("BackThrottle", &_comp.backThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("VerticalThrottle", &_comp.verticalThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("BoostFuelReserve", &_comp.boostFuelReserve, 0.5f, 0.0f);
		ImGui::DragFloat("DashInterval", &_comp.dashInterval, 0.05f, 0.0f);
		ImGui::DragFloat("DashIntervalRand", &_comp.dashIntervalRand, 0.05f, 0.0f);

		ImGui::SeparatorText("Gun");
		ImGui::DragFloat("GunRange", &_comp.gunRange, 1.0f, 0.0f);
		ImGui::DragFloat("GunConeDeg", &_comp.gunConeDeg, 1.0f, 0.0f, 180.0f);
		ImGui::DragFloat("GunBurstTime", &_comp.gunBurstTime, 0.05f, 0.0f);
		ImGui::DragFloat("GunRestTime", &_comp.gunRestTime, 0.05f, 0.0f);

		ImGui::SeparatorText("Aim");
		ImGui::DragFloat("AimOffsetY", &_comp.aimOffsetY, 0.05f);
		ImGui::DragFloat("AimLeadScale", &_comp.aimLeadScale, 0.05f, 0.0f, 3.0f);

		ImGui::SeparatorText("Missile");
		ImGui::DragFloat("MissileRange", &_comp.missileRange, 1.0f, 0.0f);
		ImGui::DragFloat("MissileInterval", &_comp.missileInterval, 0.1f, 0.0f);
		ImGui::DragFloat("MissileIntervalRand", &_comp.missileIntervalRand, 0.1f, 0.0f);

		// ここから下は毎フレーム上書きされるので表示のみ
		ImGui::SeparatorText("Runtime");
		static const char* _patternName[] = {
			"Standoff", "Rush", "HighGround", "LowGround", "Orbit", "Retreat" };
		static const char* _maneuverName[] = { "Wait", "Approach", "Keep", "Back", "Hold" };

		ImGui::Text("Pattern  : %s (next %.2f s)",
			_patternName[static_cast<int>(_comp.pattern)], _comp.patternTimer);
		ImGui::Text("Maneuver : %s", _maneuverName[static_cast<int>(_comp.maneuver)]);
		ImGui::Text("Distance : %.2f m", _comp.distance);
		ImGui::Text("Strafe   : %+.0f (next %.2f s)", _comp.strafeSign, _comp.strafeTimer);
		ImGui::Text("Hold     : %.2f s", _comp.strafeHoldTimer);
		ImGui::Text("Gun      : %s (next %.2f s)", _comp.isGunActive ? "fire" : "rest", _comp.gunTimer);
		ImGui::Text("Missile  : next %.2f s", _comp.missileTimer);
	}
};
