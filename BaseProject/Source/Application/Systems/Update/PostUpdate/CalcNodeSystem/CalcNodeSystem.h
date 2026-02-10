#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class CalcNodeSystem : public SystemBase<CalcNodeSystem>
{
public:

	static constexpr SystemType s_type = SystemType::PostUpdate;

	void Run(World& a_world, float a_dt);
};