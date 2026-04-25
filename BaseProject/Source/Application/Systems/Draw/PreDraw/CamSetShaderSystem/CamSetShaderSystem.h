#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

class CamSetShaderSystem : public Engine::ECS::SystemBase<CamSetShaderSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PreDraw;

	void Init(Engine::ECS::World& a_world) override;
};