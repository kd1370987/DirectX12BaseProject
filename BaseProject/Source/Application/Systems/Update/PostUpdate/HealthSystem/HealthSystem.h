#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 受けたヒットぶん体力を減らし、0 になったら自分を消すシステム。
class HealthSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
