#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// AnimatorAsset が持つ加算ポーズのボーン定義を、
// モデルのノードインデックスへ解決してプールへ展開する。
class AdditivePoseLinkSystem : public Engine::ECS::SystemBase<AdditivePoseLinkSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::Start;

	void Init(Engine::ECS::World& a_world) override;
};
