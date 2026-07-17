#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class RegisterAnimatedRayWorldSystem : public Engine::ECS::SystemBase<RegisterAnimatedRayWorldSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Draw;

	void Init(Engine::ECS::World& a_world) override;
};