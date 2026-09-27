#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 受けたヒットぶん体力を減らし、0 になったら自分を消すシステム。
class HealthSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
