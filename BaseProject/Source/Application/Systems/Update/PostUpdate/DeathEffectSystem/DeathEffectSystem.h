#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 死んだエンティティの DeathEffectComponent を見て、
// 登録されている EffectAsset をその場で再生するシステム。
class DeathEffectSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
