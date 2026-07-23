#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// パラメータから遷移を評価し、現在ステートを確定する
class ActionStateCommitSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
