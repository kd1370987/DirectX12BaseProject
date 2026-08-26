#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// FlyingSoundComponent を持つエンティティ(ミサイル等)の飛翔音を、
// その位置で3D再生し続けるシステム。消えたエンティティのボイスも回収する。
class FlyingSoundSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
