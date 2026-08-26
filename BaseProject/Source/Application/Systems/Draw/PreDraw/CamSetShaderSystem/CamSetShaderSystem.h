#pragma once

#include "Application/ECS/ISystem/ISystem.h"

class CamSetShaderSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};