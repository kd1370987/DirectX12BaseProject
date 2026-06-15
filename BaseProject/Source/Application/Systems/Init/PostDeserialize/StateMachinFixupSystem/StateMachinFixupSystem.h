#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class StateMachinFixupSystem : public Engine::ECS::SystemBase<StateMachinFixupSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;

	void Init(Engine::ECS::World& a_world) override;
};