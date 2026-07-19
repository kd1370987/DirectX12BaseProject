#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// OBBコライダーのテスト用システム
// コリジョンワールドと重なり判定し、結果をデバッグワイヤーで表示する
class OBBCollisionSystem : public Engine::ECS::SystemBase<OBBCollisionSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Physics;

	void Init(Engine::ECS::World& a_world) override;
};
