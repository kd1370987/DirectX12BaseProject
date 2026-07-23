#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 球コライダー＋CollisionEvent を持つエンティティ(弾など)が、
// コリジョンワールドへ VsSphere で問い合わせ、当たった相手・自分双方の
// CollisionEvent を埋めるシステム。
class HitDetectSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
