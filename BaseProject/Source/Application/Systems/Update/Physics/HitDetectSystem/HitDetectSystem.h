#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 球コライダー＋CollisionEvent を持つエンティティ(弾など)が、
// コリジョンワールドへ VsSphere で問い合わせ、当たった相手・自分双方の
// CollisionEvent を埋めるシステム。
class HitDetectSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
