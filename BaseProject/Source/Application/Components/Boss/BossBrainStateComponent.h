#pragma once

namespace App::Component
{
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

	//==========================================================================================
	// BossBrainStateComponent
	//
	// ボスの思考の状態(行動パターン・機動フェーズ・各タイマー)。
	// 書くのは BossCombatIntentSystem だけで、死亡中は DeathBossOrderGateSystem が撃つ番を下ろす。
	//
	// ・BossParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
	//==========================================================================================
	struct BossBrainStateComponent
	{
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
		float distance         = 0.0f;		// 相手までの距離(m。表示用)
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::BossBrainStateComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::BossBrainStateComponent& _comp = Engine::EditorField::GetValue<App::Component::BossBrainStateComponent>(a_context.pData);

		// 毎フレーム上書きされるので表示のみ
		static const char* PATTERN_NAME[] = {
			"Standoff", "Rush", "HighGround", "LowGround", "Orbit", "Retreat" };
		static const char* MANEUVER_NAME[] = { "Wait", "Approach", "Keep", "Back", "Hold" };

		Engine::EditorField::Value("Pattern", "%s (next %.2f s)", PATTERN_NAME[static_cast<int>(_comp.pattern)], _comp.patternTimer);
		Engine::EditorField::Value("Maneuver", "%s", MANEUVER_NAME[static_cast<int>(_comp.maneuver)]);
		Engine::EditorField::Value("Distance", "%.2f m", _comp.distance);
		Engine::EditorField::Value("Strafe", "%+.0f (next %.2f s)", _comp.strafeSign, _comp.strafeTimer);
		Engine::EditorField::Value("Hold", "%.2f s", _comp.strafeHoldTimer);
		Engine::EditorField::Value("Gun", "%s (next %.2f s)", _comp.isGunActive ? "fire" : "rest", _comp.gunTimer);
		Engine::EditorField::Value("Missile", "next %.2f s", _comp.missileTimer);
	}
};
