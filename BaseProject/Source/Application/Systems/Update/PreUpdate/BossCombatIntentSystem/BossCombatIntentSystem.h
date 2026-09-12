#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 人型ボスの「行動決定」。プレイヤーの入力(InputMoveSystem / InputActionSystem)に相当する。
// 視点角・移動入力・ブースト入力・発射入力・狙点を作るだけで、実際に動かすのは既存のシステム。
class BossCombatIntentSystem : public App::ECS::APPISystem
{
public:
	void Init(App::ECS::APPWorld& a_world) override;
};
