#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 敵の「撃つ / 撃たない」を索敵結果から決めて ActionIntentComponent へ書くシステム。
// プレイヤーの InputActionSystem に相当する、敵側の発射入力。
class EnemyShootIntentSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
