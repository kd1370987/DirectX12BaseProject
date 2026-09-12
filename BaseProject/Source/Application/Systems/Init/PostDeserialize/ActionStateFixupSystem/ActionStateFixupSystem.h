#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// ゲームプレイ用ステートマシンの復元(GUID→ハンドル解決＋実行時インスタンス確保)
class ActionStateFixupSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
