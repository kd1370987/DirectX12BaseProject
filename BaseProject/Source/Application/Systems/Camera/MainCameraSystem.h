#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 映すカメラ(CameraParamComponent.isActive が立っているもの)を1台選び、
// SingletonEntityResource.mainCamera へ書き込むシステム。
class MainCameraSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
