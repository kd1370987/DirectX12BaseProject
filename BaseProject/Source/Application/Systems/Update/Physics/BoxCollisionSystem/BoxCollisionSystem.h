#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// ボックス(AABB)コライダーのテスト用システム
// コリジョンワールドと重なり判定し、結果をデバッグワイヤーで表示する
class BoxCollisionSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
