#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 毎フレーム先頭で HitEventResource の配列をクリアするシステム。
// 「産む前に前フレーム分を消す」ため PreUpdate に置く。
// CollisionEventClearSystem と役割は同じだが、
// こちらはコンポーネントではなくワールドリソースを触るのでカスタムタスクで登録する。
class HitEventClearSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
