#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// プレイヤー専用の旋回システム。
// ActionState(ActionNode.faceMode)を見て、進行方向を向くか狙い方向を向くかを切り替える。
class LockOnRotationSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
