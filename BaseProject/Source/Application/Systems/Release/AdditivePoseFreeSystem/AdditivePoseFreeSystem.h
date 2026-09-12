#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// 加算ポーズ用に確保したボーン配列を返却する。
// AnimationMatrixFreeSystem に相乗りさせると、加算ポーズを持たないエンティティが
// クエリから漏れてノードポーズが解放されなくなるため、独立したシステムにしている。
class AdditivePoseFreeSystem : public App::ECS::APPISystem
{
public:


	void Init(App::ECS::APPWorld& a_world) override;
};
