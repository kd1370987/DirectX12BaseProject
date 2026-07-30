#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 慣性コンポーネントを持つエンティティの移動に慣性を付与するシステム
// 慣性を持たないエンティティは PositionIntegrationSystem 側が処理する
class InertiaIntegrationSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
