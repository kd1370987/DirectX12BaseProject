#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class DynamicObjectDrawSystem : public Engine::ECS::SystemBase<DynamicObjectDrawSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Draw;

	void Init(Engine::ECS::World& a_world) override;
};