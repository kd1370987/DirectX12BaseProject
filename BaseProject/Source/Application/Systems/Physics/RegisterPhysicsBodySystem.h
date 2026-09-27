#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// コライダーを物理空間(Jolt)へ登録する(Start)。静的も動くものも
//
// 動くボディの位置合わせは SyncPhysicsBodySystem(Update)。
// 消すのは PhysicsBodyFreeSystem(Release フェーズ)
//==========================================================================================
class RegisterPhysicsBodySystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
