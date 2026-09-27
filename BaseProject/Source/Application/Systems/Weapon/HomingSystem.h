#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// HomingComponent を持つ投射物を、狙う相手(targetEntity)へ向けて曲げるシステム。
// 速度の「大きさ」は変えず、向きだけを turnSpeed の範囲で回す。
class HomingSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
