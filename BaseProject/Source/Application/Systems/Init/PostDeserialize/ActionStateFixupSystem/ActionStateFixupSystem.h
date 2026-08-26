#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// ゲームプレイ用ステートマシンの復元(GUID→ハンドル解決＋実行時インスタンス確保)
class ActionStateFixupSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
