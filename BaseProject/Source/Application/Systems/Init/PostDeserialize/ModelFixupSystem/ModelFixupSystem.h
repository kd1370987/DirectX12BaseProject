#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class ModelFixupSystem : public Engine::ECS::SystemBase
{
public:


	void Init(Engine::ECS::World& a_world) override;
};