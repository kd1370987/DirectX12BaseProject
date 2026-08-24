#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 近距離型の敵の「足を止めて撃つ / 撃たずに動き直す」を交互に回すシステム。
// 攻撃圏の内側でだけ働き、移動入力と発射入力の両方をこのシステムが持つ。
class CloseCombatIntentSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
