#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// 物理空間(Jolt)のボディを消す(Release)
//
// エンティティが消えるとき(シーンの終了・作り直しを含む)は必ずこのフェーズを通る。
// 解放フック(ComponentTraits::Release)はワールドを受け取れないので、
// シーンごとの PhysicsWorld へ返すのはこちらで行う
//==========================================================================================
class PhysicsBodyFreeSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
