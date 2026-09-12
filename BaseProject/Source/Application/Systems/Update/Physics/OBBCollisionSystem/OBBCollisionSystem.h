#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// OBBコライダーのテスト用システム
// コリジョンワールドと重なり判定し、結果をデバッグワイヤーで表示する
class OBBCollisionSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
