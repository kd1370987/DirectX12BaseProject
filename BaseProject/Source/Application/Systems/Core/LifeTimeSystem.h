#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// LifeTimeComponent を持つエンティティの残り時間を減らし、
// 尽きたら自分を消すシステム。弾・エフェクトなど種類を問わず面倒を見る。
class LifeTimeSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
