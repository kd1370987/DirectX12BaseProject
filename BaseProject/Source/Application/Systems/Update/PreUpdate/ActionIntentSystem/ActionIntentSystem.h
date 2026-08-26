#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 入力・状況をゲームプレイ用ステートマシンのパラメータへ書き込む
class ActionIntentSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
