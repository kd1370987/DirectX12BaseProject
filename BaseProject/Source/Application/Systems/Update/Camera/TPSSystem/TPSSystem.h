#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class TPSSystem : public Engine::ECS::SystemBase<TPSSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Camera;

	void Init(Engine::ECS::World& a_world) override;
};