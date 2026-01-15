#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class CamSetShaderSystem : public SystemBase<CamSetShaderSystem>
{
	public:
	static constexpr SystemType s_type = SystemType::Camera;
	void Run(World& a_world, float a_dt);
};