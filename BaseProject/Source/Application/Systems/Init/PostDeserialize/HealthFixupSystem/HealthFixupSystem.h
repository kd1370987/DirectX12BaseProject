#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 生成された HealthComponent の現在体力を最大体力で満たすシステム。
class HealthFixupSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
