#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// CollisionEvent がヒットしているエンティティのうち ExplodeOnHitComponent を持つものが、
// 当たった位置に爆発/エフェクトプレハブを生成し、必要なら自分を消すシステム。
class ExplodeOnHitSystem : public Engine::ECS::SystemBase<ExplodeOnHitSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostUpdate;

	void Init(Engine::ECS::World& a_world) override;
};
