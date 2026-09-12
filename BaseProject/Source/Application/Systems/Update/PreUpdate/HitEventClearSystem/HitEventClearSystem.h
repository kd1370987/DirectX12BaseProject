#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 毎フレーム先頭で HitEventResource の配列をクリアするシステム。
// 「産む前に前フレーム分を消す」ため PreUpdate に置く。
// CollisionEventClearSystem と役割は同じだが、
// こちらはコンポーネントではなくワールドリソースを触るのでカスタムタスクで登録する。
class HitEventClearSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
