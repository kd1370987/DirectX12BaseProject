#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 人型ボスのミサイル一斉射。溜め撃ちではなく、BossCombatIntentSystem からの
// 要求(BossComponent::isMissileRequest)を受けて発射キューを作る。
class BossMissileSalvoSystem : public App::ECS::APPISystem
{
public:
	void Init(App::ECS::APPWorld& a_world) override;
};
