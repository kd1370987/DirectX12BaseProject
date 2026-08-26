#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 攻撃を受けたエンティティ(HitSoundComponent 保持者)から被弾音を鳴らすシステム。
class HitSoundSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
