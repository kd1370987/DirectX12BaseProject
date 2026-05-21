#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class HierarchyLinkSystem : public Engine::ECS::SystemBase<HierarchyLinkSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;

	void Init(Engine::ECS::World& a_world) override;
};