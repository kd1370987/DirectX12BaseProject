#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 映すカメラ(CameraParamComponent.isActive が立っているもの)を1台選び、
// SingletonEntityResource.mainCamera へ書き込むシステム。
class MainCameraSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
