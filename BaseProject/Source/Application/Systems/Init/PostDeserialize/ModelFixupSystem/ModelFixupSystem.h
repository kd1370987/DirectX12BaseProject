#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class ModelFixupSystem : public Engine::ECS::SystemBase<ModelFixupSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;

	void Run(Engine::ECS::World& a_world, float a_dt);
};