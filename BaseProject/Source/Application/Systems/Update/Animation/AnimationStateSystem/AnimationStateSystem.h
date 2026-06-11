#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class AnimationStateSystem : public Engine::ECS::SystemBase<AnimationStateSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Animation;
	void Init(Engine::ECS::World& a_world) override;
};