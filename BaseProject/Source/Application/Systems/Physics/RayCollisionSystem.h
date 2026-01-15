#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RayCollisionSystem : public SystemBase<RayCollisionSystem>
{
public:
	static constexpr SystemType s_type = SystemType::Physics;
	void Run(World& a_world, float a_dt);
};