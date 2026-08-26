#pragma once

#include "Application/ECS/ISystem/ISystem.h"

class InputMoveSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};