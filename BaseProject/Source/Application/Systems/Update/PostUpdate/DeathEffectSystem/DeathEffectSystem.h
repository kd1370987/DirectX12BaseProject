#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 死んだエンティティの DeathEffectComponent を見て、
// 登録されている EffectAsset をその場で再生するシステム。
class DeathEffectSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
