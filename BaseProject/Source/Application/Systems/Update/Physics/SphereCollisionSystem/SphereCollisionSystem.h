#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 球コライダーのテスト用システム
// 球をコリジョンワールドから押し出し、結果をデバッグワイヤーで表示する
class SphereCollisionSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
