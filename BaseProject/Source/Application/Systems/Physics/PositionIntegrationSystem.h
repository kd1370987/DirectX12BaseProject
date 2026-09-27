#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

class PositionIntegrationSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};