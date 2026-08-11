#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// FlyingSoundComponent を持つエンティティ(ミサイル等)の飛翔音を、
// その位置で3D再生し続けるシステム。消えたエンティティのボイスも回収する。
class FlyingSoundSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
