#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class AnimationOptionalDrawSystem : public Engine::ECS::SystemBase<AnimationOptionalDrawSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Draw;

	void Init(Engine::ECS::World& a_world) override;
};