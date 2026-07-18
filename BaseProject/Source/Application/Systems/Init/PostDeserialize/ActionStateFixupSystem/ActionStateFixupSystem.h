#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// ゲームプレイ用ステートマシンの復元(GUID→ハンドル解決＋実行時インスタンス確保)
class ActionStateFixupSystem : public Engine::ECS::SystemBase<ActionStateFixupSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PostDeserialize;

	void Init(Engine::ECS::World& a_world) override;
};
