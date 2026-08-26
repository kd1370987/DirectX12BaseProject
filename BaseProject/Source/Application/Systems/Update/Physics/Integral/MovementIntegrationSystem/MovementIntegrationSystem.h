#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// MovementComponent を持つエンティティの速度を加速度/減速度で目標速度へ追従させ、
// その実速度で座標を進めるシステム。
// MovementComponent を持たないエンティティは PositionIntegrationSystem 側が処理する
class MovementIntegrationSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
