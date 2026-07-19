#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// カプセルコライダーのテスト用システム
// カプセルをコリジョンワールドに問い合わせ、結果をデバッグワイヤーで表示する
class CapsuleCollisionSystem : public Engine::ECS::SystemBase<CapsuleCollisionSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Physics;

	void Init(Engine::ECS::World& a_world) override;
};
