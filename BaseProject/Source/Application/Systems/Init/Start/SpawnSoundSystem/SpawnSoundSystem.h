#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// SoundComponent の isPlayOnSpawn が立っているエンティティの音を、
// 湧いた瞬間(Startフェーズ)に一度だけ鳴らすシステム。
class SpawnSoundSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
