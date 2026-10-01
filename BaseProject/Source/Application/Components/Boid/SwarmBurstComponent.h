#pragma once

#include "Engine/Editor/Helper/EditorField.h"

//==========================================================================================
// SwarmBurstComponent
//
// ワームボスが死ぬときに爆散して、外へ飛び散っているボイドの状態。
//
// ・付けるのは SwarmBossController(死亡の最後に、体のボイドから群れの部品を外して付ける)。
//   プレハブには入れない。
// ・飛ばすのも、時間が来たら落とすのも SwarmBurstSystem。
//   落とすのは自分にダメージを積む形なので、死亡エフェクトはボイドの DeathEffectComponent のまま出る。
// ・SwarmBossBoidTag は残すので、落ちるまではボスの体力に数えられ、落ちたぶんだけ減って最後に 0 になる。
// ・重力と減速は爆散を頼んだ死亡の行動の調整値を写したもの。実行中の値なので保存しない。
//==========================================================================================
struct SwarmBurstComponent
{
	Math::Vector3 velocity = {};	// 今の速度(m/秒)
	float timer   = 0.0f;			// 落ちるまでの残り時間(秒)
	float gravity = 0.0f;			// 重力(m/秒^2)
	float drag    = 0.0f;			// 減速(1/秒)
	bool isExploded = false;		// 落とし済みか(死亡状態へ入るのを待っている)
};

template<>
struct Engine::ECS::ComponentTraits<SwarmBurstComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		SwarmBurstComponent& _comp = Engine::Editor::GetValue<SwarmBurstComponent>(a_context.pData);

		// 毎フレーム書き換わる値なので表示のみ
		Engine::Editor::Value("Velocity", "%.1f, %.1f, %.1f", _comp.velocity.x, _comp.velocity.y, _comp.velocity.z);
		Engine::Editor::Value("Timer", "%.2f s", _comp.timer);
		Engine::Editor::Value("Exploded", "%s", _comp.isExploded ? "true" : "false");
	}
};
