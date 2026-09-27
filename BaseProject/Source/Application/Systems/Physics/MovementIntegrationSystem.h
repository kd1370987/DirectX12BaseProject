#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// MovementParamsComponent を持つエンティティの速度を加速度/減速度で目標速度へ追従させ、
// その実速度で座標を進めるシステム。
// MovementParamsComponent を持たないエンティティは PositionIntegrationSystem 側が処理する
class MovementIntegrationSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
