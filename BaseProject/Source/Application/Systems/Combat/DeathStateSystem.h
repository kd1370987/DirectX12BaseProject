#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 死亡状態(HealthComponent.isDead)の面倒を見るシステム。
// 死んでいるあいだ入力/AIを止め、指定秒たったら解放予約する。
class DeathStateSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
