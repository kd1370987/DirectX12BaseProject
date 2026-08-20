#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// ブースターの噴射エフェクトの置き方と、吹かした瞬間の膨らみを
// EffectAssetComponent へ書き込むシステム。
class BoosterEffectSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
