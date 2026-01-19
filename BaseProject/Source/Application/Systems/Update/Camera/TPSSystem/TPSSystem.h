#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class TPSSystem : public SystemBase<TPSSystem>
{
public:

	static constexpr SystemType s_type = SystemType::Camera;

	void Run(World& a_world, float a_dt);
};