#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 球コライダーのテスト用システム
// 球をコリジョンワールドから押し出し、結果をデバッグワイヤーで表示する
class SphereCollisionSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
