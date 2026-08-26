#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// AnimatorAsset が持つ加算ポーズのボーン定義を、
// モデルのノードインデックスへ解決してプールへ展開する。
class AdditivePoseLinkSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};
