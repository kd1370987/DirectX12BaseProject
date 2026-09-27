#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 死んだエンティティの DeathEffectComponent を見て、
// 登録されている EffectAsset をその場で再生するシステム。
class DeathEffectSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
