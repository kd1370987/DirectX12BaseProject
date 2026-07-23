#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// カプセルコライダーのテスト用システム
// カプセルをコリジョンワールドに問い合わせ、結果をデバッグワイヤーで表示する
class CapsuleCollisionSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
