#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class StateMachineCommitSystem : public Engine::ECS::SystemBase
{
public:


	void Init(Engine::ECS::World& a_world) override;
};