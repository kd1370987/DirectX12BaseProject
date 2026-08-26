#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// ブースターの噴射エフェクトの置き方と、吹かした瞬間の膨らみを
// EffectAssetComponent へ書き込むシステム。
class BoosterEffectSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
