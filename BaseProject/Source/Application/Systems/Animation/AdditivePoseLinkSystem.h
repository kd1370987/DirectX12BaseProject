#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// AnimatorAsset が持つ加算ポーズのボーン定義を、
// モデルのノードインデックスへ解決してプールへ展開する。
class AdditivePoseLinkSystem : public App::ECS::APPISystem
{
public:


	void Init(App::ECS::APPWorld& a_world) override;
};
