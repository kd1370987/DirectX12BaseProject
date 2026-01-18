#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RotationSystem : public SystemBase<RotationSystem>
{
public:

	static constexpr SystemType s_type = SystemType::Update;

	void Run(World& a_world, float a_dt);
};