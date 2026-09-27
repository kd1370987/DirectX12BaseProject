#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// ブースターの噴射エフェクトの置き方と、吹かした瞬間の膨らみを
// EffectAssetComponent へ書き込むシステム。
class BoosterEffectSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
