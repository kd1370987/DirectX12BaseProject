#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// AnimatorFreeSystem
//
// アニメーターの遷移パラメータの実体(ItemPool<StateMachineInstance>)をプールへ返す。
// 実体はワールドのリソースにあり、コンポーネントの解放フック(エンジンのサービスしか受け取らない)
// からは触れないので、Release フェーズのシステムで返す。
// 以前はどこからも返しておらず、エンティティを消すたびにプールへ残っていた。
//==========================================================================================
class AnimatorFreeSystem : public App::ECS::APPISystem
{
public:
	void Init(App::ECS::APPWorld& a_world) override;
};
