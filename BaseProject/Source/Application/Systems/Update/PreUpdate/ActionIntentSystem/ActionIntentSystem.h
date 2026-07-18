#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 入力・状況をゲームプレイ用ステートマシンのパラメータへ書き込む
class ActionIntentSystem : public Engine::ECS::SystemBase<ActionIntentSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PreUpdate;

	void Init(Engine::ECS::World& a_world) override;
};
