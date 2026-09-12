#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// SoundComponent の isPlayOnSpawn が立っているエンティティの音を、
// 湧いた瞬間(Startフェーズ)に一度だけ鳴らすシステム。
class SpawnSoundSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
