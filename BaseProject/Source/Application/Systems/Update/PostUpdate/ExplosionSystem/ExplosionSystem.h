#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// ExplosionComponent の経過時間を進め、時間が来たパーツのプレハブを
// 自分と同じ場所に生成する。全パーツを出し終えたら自分を消すシステム。
class ExplosionSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
