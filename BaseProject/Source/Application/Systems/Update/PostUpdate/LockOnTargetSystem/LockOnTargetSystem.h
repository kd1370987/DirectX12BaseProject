#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// プレイヤーのレティクル内に居る敵を集めて、画面中央に最も近いものをロックするシステム。
// 結果は LockOnTargetComponent へ書き、HUD(枠の表示)と旋回(体の向き)が読む。
class LockOnTargetSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
