#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class GUIDFixupSystem : public Engine::ECS::SystemBase<GUIDFixupSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;

	void Init(Engine::ECS::World& a_world) override;
};