#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// CollisionEvent がヒットしているエンティティのうち ExplodeOnHitComponent を持つものが、
// 当たった位置に爆発/エフェクトプレハブを生成し、必要なら自分を消すシステム。
class ExplodeOnHitSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
