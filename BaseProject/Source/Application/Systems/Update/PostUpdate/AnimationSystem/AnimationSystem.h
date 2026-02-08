#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class AnimationSystem : public SystemBase<AnimationSystem>
{
public:

	static constexpr SystemType s_type = SystemType::PostUpdate;

	void Run(World& a_world, float a_dt);
};