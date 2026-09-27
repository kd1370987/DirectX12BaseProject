#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 毎フレーム先頭で CollisionEvent をクリア(other=INVALID)するシステム。
// 「産む前に前フレーム分を消す」ため PreUpdate に置く。
class CollisionEventClearSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
