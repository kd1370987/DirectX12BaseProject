#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// CameraParamComponent.isDirty が立っているカメラの射影行列を作り直すシステム。
// スピードに応じた画角の変化(TPSSystem が fovBoost を書く)を反映するために使う。
class CameraProjUpdateSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
