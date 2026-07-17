#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RayAnimationModelSystem : public Engine::ECS::SystemBase<RayAnimationModelSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Start;

	void Init(Engine::ECS::World& a_world) override;
};