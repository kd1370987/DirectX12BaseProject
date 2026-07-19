#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 球コライダーのテスト用システム
// 球をコリジョンワールドから押し出し、結果をデバッグワイヤーで表示する
class SphereCollisionSystem : public Engine::ECS::SystemBase<SphereCollisionSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Physics;

	void Init(Engine::ECS::World& a_world) override;
};
