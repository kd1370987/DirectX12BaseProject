#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// 静的コライダーを物理空間(Jolt)へ登録する(Start)
//
// 移行中は RegisterCollisionWorldSystem と並べて動かし、同じものを両方の空間へ入れる。
// 消すのは PhysicsBodyFreeSystem(Release フェーズ)
//==========================================================================================
class RegisterPhysicsBodySystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
