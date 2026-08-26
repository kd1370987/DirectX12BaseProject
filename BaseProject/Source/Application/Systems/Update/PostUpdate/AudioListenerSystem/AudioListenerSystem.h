#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// AudioListenerComponent を持つエンティティ(プレイヤー)の位置・向きを
// 毎フレーム AudioManager へ送るシステム。
class AudioListenerSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
