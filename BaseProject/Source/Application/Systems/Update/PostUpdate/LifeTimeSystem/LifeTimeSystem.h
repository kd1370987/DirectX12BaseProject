#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// LifeTimeComponent を持つエンティティの残り時間を減らし、
// 尽きたら自分を消すシステム。弾・エフェクトなど種類を問わず面倒を見る。
class LifeTimeSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
