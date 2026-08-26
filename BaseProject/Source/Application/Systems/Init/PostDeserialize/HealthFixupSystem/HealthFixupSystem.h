#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 生成された HealthComponent の現在体力を最大体力で満たすシステム。
class HealthFixupSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
