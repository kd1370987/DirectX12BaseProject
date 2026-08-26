#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 毎フレーム先頭で HitEventResource の配列をクリアするシステム。
// 「産む前に前フレーム分を消す」ため PreUpdate に置く。
// CollisionEventClearSystem と役割は同じだが、
// こちらはコンポーネントではなくワールドリソースを触るのでカスタムタスクで登録する。
class HitEventClearSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
