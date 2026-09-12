#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

class StateMachineCommitSystem : public App::ECS::APPISystem
{
public:


	void Init(App::ECS::APPWorld& a_world) override;
};