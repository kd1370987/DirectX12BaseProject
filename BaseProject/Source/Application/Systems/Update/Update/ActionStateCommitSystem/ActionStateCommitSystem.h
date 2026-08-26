#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// パラメータから遷移を評価し、現在ステートを確定する
class ActionStateCommitSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
