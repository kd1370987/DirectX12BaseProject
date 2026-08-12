#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 死んだエンティティの DeathEffectComponent を見て、
// 登録されているエフェクトプレハブをその場に生成するシステム。
class DeathEffectSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
