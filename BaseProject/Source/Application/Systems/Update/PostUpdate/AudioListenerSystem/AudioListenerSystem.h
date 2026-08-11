#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// AudioListenerComponent を持つエンティティ(プレイヤー)の位置・向きを
// 毎フレーム AudioManager へ送るシステム。
class AudioListenerSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
