#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// SoundComponent の isPlayOnSpawn が立っているエンティティの音を、
// 湧いた瞬間(Startフェーズ)に一度だけ鳴らすシステム。
class SpawnSoundSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
