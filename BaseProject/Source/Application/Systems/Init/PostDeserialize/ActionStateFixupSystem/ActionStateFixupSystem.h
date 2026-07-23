#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// ゲームプレイ用ステートマシンの復元(GUID→ハンドル解決＋実行時インスタンス確保)
class ActionStateFixupSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
