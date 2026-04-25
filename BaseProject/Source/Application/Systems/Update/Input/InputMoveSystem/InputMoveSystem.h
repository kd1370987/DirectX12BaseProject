#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class InputMoveSystem : public Engine::ECS::SystemBase<InputMoveSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Update;

	void Init(Engine::ECS::World& a_world) override;
};