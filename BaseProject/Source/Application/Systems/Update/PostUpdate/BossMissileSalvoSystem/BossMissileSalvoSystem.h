#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 人型ボスのミサイル一斉射。溜め撃ちではなく、BossCombatIntentSystem からの
// 要求(BossComponent::isMissileRequest)を受けて発射キューを作る。
class BossMissileSalvoSystem : public Engine::ECS::SystemBase
{
public:
	void Init(Engine::ECS::World& a_world) override;
};
