#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class ScreenUIDrawSystem : public SystemBase<ScreenUIDrawSystem>
{
public:

	static constexpr SystemType s_type = SystemType::Draw;

	void Run(World& a_world, float a_dt);
};