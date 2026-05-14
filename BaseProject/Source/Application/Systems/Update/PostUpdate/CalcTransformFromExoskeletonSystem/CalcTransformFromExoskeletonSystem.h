#pragma once
#include "Engine/ECS/System/SystemBase/SystemBase.h"

class CalccTransformFromExoskeletonSystem : public Engine::ECS::SystemBase<CalccTransformFromExoskeletonSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostUpdate;
	void Init(Engine::ECS::World& a_world) override;
};