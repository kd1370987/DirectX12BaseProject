#pragma once

#include "Application/ECS/ISystem/ISystem.h"

class StaticObjectDrawSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};