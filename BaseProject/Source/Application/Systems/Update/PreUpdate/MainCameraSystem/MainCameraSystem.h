#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 映すカメラ(CameraParamComponent.isActive が立っているもの)を1台選び、
// SingletonEntityResource.mainCamera へ書き込むシステム。
class MainCameraSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
