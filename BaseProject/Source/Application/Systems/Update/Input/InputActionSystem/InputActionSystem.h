#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class InputActionSystem : public Engine::ECS::SystemBase<InputActionSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Input;

	void Init(Engine::ECS::World& a_world) override;
};