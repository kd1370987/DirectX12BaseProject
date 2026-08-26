#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// CameraParamComponent.isDirty が立っているカメラの射影行列を作り直すシステム。
// スピードに応じた画角の変化(TPSSystem が fovBoost を書く)を反映するために使う。
class CameraProjUpdateSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
