#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class AnimationSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};