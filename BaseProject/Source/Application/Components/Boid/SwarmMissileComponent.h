#pragma once

#include "Engine/Editor/Helper/EditorField.h"

//==========================================================================================
// SwarmMissileComponent
//
// ワームの体から切り離され、プレイヤーへ飛んでいく自爆ミサイルになったボイドの状態。
//
// ・付けるのは SwarmMissileSystem(切り離し)。ワームボスの巻き付き攻撃(CoilAttack)が
//   「どの小隊長から1体切り離すか」を SwarmMissileResource へ積み、それを受けて
//   ボイドの群れの部品(操舵・追従・ウェーブ・体当たり)を外し、これを付ける。
//   プレハブには入れない。
// ・SwarmBossBoidTag は残すので、飛んでいる間もボスの体力に数えられる。
//   自爆しても撃ち落とされても、死んだ時点でボスの体力が1減る。
// ・飛び方の調整値は SwarmMissileResource にまとめてあり、全ミサイルで共有する。
//   ここが持つのは1体ずつ変わる値だけ。実行中の値なので保存しない。
//==========================================================================================
struct SwarmMissileComponent
{
	Math::Vector3 dir = { 0.0f, 1.0f, 0.0f };	// 進んでいる向き(単位ベクトル)
	float speed = 0.0f;							// 今の速さ(m/秒)
	float time = 0.0f;							// 切り離されてからの経過時間(秒)
	bool isExploded = false;					// 自爆済みか(死亡状態へ入るのを待っている)
};

template<>
struct Engine::ECS::ComponentTraits<SwarmMissileComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		SwarmMissileComponent& _comp = Engine::Editor::GetValue<SwarmMissileComponent>(a_context.pData);

		// 毎フレーム書き換わる値なので表示のみ
		Engine::Editor::Value("Dir", "%.2f, %.2f, %.2f", _comp.dir.x, _comp.dir.y, _comp.dir.z);
		Engine::Editor::Value("Speed", "%.1f m/s", _comp.speed);
		Engine::Editor::Value("Time", "%.2f s", _comp.time);
		Engine::Editor::Value("Exploded", "%s", _comp.isExploded ? "true" : "false");
	}
};
