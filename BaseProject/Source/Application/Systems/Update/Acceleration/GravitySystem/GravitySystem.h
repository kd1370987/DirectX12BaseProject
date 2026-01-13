#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class GravitySystem : public SystemBase<GravitySystem>
{
public:
	static constexpr SystemType s_type = SystemType::Update;

	void Run(World& a_world, float a_dt);
};