#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 受けたヒットぶん体力を減らし、0 になったら自分を消すシステム。
class HealthSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
