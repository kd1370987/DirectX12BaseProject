#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class FollowTargetLinkSystem : public Engine::ECS::SystemBase<FollowTargetLinkSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;

	void Init(Engine::ECS::World& a_world) override;
};