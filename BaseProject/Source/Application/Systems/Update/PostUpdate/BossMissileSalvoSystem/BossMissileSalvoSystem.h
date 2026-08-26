#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 人型ボスのミサイル一斉射。溜め撃ちではなく、BossCombatIntentSystem からの
// 要求(BossComponent::isMissileRequest)を受けて発射キューを作る。
class BossMissileSalvoSystem : public App::ECS::ISystem
{
public:
	void Init(App::ECS::World& a_world) override;
};
