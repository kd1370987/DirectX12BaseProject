#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// プレイヤー専用の旋回システム。
// ActionState(ActionNode.faceMode)を見て、進行方向を向くか狙い方向を向くかを切り替える。
class LockOnRotationSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
