#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 毎フレーム先頭で CollisionEvent をクリア(other=INVALID)するシステム。
// 「産む前に前フレーム分を消す」ため PreUpdate に置く。
class CollisionEventClearSystem : public Engine::ECS::SystemBase<CollisionEventClearSystem>
{
public:
	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PreUpdate;

	void Init(Engine::ECS::World& a_world) override;
};
