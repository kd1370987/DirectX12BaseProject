#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// FlyingSoundComponent を持つエンティティ(ミサイル等)の飛翔音を、
// その位置で3D再生し続けるシステム。消えたエンティティのボイスも回収する。
class FlyingSoundSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
