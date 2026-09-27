#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 生成された HealthComponent の現在体力を最大体力で満たすシステム。
class HealthFixupSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
