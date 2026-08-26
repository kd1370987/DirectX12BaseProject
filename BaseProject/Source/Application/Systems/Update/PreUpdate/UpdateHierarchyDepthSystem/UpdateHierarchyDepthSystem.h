#pragma once

#include "Application/ECS/ISystem/ISystem.h"

class UpdateHierarchyDepthSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};