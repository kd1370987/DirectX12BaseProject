#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class PositionIntegrationSystem : public Engine::ECS::SystemBase<PositionIntegrationSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Update;

	void Run(Engine::ECS::World& a_world, float a_dt);
};