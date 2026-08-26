#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 毎フレーム先頭で CollisionEvent をクリア(other=INVALID)するシステム。
// 「産む前に前フレーム分を消す」ため PreUpdate に置く。
class CollisionEventClearSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
