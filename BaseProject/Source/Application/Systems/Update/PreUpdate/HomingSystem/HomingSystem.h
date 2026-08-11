#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// HomingComponent を持つ投射物を、狙う相手(targetEntity)へ向けて曲げるシステム。
// 速度の「大きさ」は変えず、向きだけを turnSpeed の範囲で回す。
class HomingSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
