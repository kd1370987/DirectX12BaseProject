#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 加算ポーズ用に確保したボーン配列を返却する。
// AnimationMatrixFreeSystem に相乗りさせると、加算ポーズを持たないエンティティが
// クエリから漏れてノードポーズが解放されなくなるため、独立したシステムにしている。
class AdditivePoseFreeSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};
