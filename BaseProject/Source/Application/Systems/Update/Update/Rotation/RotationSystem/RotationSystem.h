#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RotationSystem : public Engine::ECS::SystemBase<RotationSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Update;
	void Init(Engine::ECS::World& a_world) override;
	void Run(Engine::ECS::World& a_world, float a_dt);
};