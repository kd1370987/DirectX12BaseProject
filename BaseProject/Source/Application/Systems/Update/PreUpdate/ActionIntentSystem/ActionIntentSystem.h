#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 入力・状況をゲームプレイ用ステートマシンのパラメータへ書き込む
class ActionIntentSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
