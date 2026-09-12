#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// パラメータから遷移を評価し、現在ステートを確定する
class ActionStateCommitSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
