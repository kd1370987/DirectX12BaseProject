#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RayCollisionSystem : public Engine::ECS::SystemBase<RayCollisionSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Physics;

	void Init(Engine::ECS::World& a_world) override;
};