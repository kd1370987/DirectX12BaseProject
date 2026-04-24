#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class SkinningSystem : public Engine::ECS::SystemBase<SkinningSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostUpdate;
	void Init(Engine::ECS::World& a_world) override;
	void Run(Engine::ECS::World& a_world, float a_dt);
};