#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// EffectEventsComponent の対応表を見て、出来事(生まれた・死んだ・攻撃を受けた)が起きたら
// 登録されている EffectAsset をその場に出すシステム。
// 対応表のハンドルの解決と先読み(Fixup)もここで受け持つ。
class EffectEventSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
