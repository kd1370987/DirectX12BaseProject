#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 攻撃を受けたエンティティ(HitSoundComponent 保持者)から被弾音を鳴らすシステム。
class HitSoundSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
