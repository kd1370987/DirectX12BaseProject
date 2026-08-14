#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

//==========================================================================================
// モデルの到着待ちゲート
//
// 非同期ロード中のモデルを持つエンティティを Start フェーズへ進めないようにする。
// 実際に待たせる処理は World::BeginFrame() の遷移側にあり、
// ここは「まだ届いていないもの」を ResourceWaitResource へ登録するだけ。
//
// 遷移より前に走る必要があるため、Awake フェーズの先頭付近に登録すること
//==========================================================================================
class ModelReadyGateSystem : public Engine::ECS::SystemBase
{
public:


	void Init(Engine::ECS::World& a_world) override;
};
