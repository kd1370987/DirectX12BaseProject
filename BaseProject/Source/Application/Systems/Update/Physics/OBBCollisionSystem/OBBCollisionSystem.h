#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// OBBコライダーのテスト用システム
// コリジョンワールドと重なり判定し、結果をデバッグワイヤーで表示する
class OBBCollisionSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
