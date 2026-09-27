#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// CollisionEvent がヒットしているエンティティのうち ExplodeOnHitComponent を持つものが、
// 当たった位置に爆発/エフェクトプレハブを生成し、必要なら自分を消すシステム。
class ExplodeOnHitSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
