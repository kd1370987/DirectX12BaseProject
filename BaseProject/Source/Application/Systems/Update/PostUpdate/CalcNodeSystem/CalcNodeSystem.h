#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class CalcNodeSystem : public Engine::ECS::SystemBase<CalcNodeSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostUpdate;
	void Init(Engine::ECS::World& a_world) override;
};