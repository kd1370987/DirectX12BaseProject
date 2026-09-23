#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

class BoidGroundEffectSystem : public App::ECS::APPISystem
{
public:
	void Init(App::ECS::APPWorld& a_world) override;
};
