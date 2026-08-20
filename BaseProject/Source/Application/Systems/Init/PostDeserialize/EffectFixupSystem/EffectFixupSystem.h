#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class EffectFixupSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
