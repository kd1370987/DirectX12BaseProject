#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class StateMachinComitSystem : public Engine::ECS::SystemBase<StateMachinComitSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Update;

	void Init(Engine::ECS::World& a_world) override;
};