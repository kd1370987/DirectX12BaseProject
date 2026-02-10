#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class SkinningSystem : public SystemBase<SkinningSystem>
{
public:

	static constexpr SystemType s_type = SystemType::PostUpdate;

	void Run(World& a_world, float a_dt);
};