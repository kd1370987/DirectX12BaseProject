#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 球コライダーのテスト用システム
// 球をコリジョンワールドから押し出し、結果をデバッグワイヤーで表示する
class SphereCollisionSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
