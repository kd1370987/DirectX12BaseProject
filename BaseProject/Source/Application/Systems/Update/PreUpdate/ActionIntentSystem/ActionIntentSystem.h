#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 入力・状況をゲームプレイ用ステートマシンのパラメータへ書き込む
class ActionIntentSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
