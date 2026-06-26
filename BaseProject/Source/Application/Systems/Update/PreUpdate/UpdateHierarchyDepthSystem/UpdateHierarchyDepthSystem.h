#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class UpdateHierarchyDepthSystem : public Engine::ECS::SystemBase<UpdateHierarchyDepthSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PreUpdate;

	void Init(Engine::ECS::World& a_world) override;
};