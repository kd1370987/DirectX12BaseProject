#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// ProjectileComponent を持つ投射物の生存時間(秒)を減らし、
// 尽きたら自分を消すだけのシステム。
class ProjectileLifeTimeSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
