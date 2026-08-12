#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// ExplosionComponent の経過時間を進め、時間が来たパーツのプレハブを
// 自分と同じ場所に生成する。全パーツを出し終えたら自分を消すシステム。
class ExplosionSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
