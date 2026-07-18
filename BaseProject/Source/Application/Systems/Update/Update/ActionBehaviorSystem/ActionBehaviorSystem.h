#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 現在のゲームプレイ状態(ActionNode)を見て、実際の行動(移動)を制御する。
// CharactorMovementSystem が速度を決めた後に走り、状態に応じて上書きする。
class ActionBehaviorSystem : public Engine::ECS::SystemBase<ActionBehaviorSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Update;

	void Init(Engine::ECS::World& a_world) override;
};
