#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// カプセルコライダーのテスト用システム
// カプセルをコリジョンワールドに問い合わせ、結果をデバッグワイヤーで表示する
class CapsuleCollisionSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
