#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class SimpleDrawSystem : public SystemBase<SimpleDrawSystem>
{
public:

	static constexpr SystemType s_type = SystemType::Draw;

	void Run(World& a_world, float a_dt);
};