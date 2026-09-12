#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 現在のゲームプレイ状態(ActionNode)を見て、実際の行動(移動)を制御する。
// CharacterMovementSystem が速度を決めた後に走り、状態に応じて上書きする。
class ActionBehaviorSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
