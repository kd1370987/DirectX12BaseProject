#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 自機の速さ(TPSCameraStateComponent::currentSpeed01)から、
// ラジアルブラーの引きずり量を作るシステム。
// 画角の広がり(TPSSystem が fovBoost を書く)と同じ元から効かせて足並みを揃える。
class RadialBlurSpeedSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
