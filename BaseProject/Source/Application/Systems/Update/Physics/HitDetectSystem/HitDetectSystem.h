#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 球コライダー＋CollisionEvent を持つエンティティ(弾など)が、
// コリジョンワールドへ VsSphere で問い合わせ、当たった相手・自分双方の
// CollisionEvent を埋めるシステム。
class HitDetectSystem : public Engine::ECS::SystemBase<HitDetectSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Physics;

	void Init(Engine::ECS::World& a_world) override;
};
