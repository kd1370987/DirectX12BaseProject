#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RegisterRayWorldSystem : public Engine::ECS::SystemBase<RegisterRayWorldSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Draw;

	void Init(Engine::ECS::World& a_world) override;
};