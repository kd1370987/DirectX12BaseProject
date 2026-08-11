#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 攻撃を受けたエンティティ(HitSoundComponent 保持者)から被弾音を鳴らすシステム。
class HitSoundSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
