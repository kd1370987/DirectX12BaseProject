#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// LifeTimeComponent を持つエンティティの残り時間を減らし、
// 尽きたら自分を消すシステム。弾・エフェクトなど種類を問わず面倒を見る。
class LifeTimeSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
