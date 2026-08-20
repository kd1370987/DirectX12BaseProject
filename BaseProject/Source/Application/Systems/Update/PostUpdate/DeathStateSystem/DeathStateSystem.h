#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 死亡状態(HealthComponent.isDead)の面倒を見るシステム。
// 死んでいるあいだ入力/AIを止め、指定秒たったら解放予約する。
class DeathStateSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
