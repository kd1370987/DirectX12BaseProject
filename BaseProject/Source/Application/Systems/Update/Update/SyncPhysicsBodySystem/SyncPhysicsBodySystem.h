#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// 動くコライダーのボディ(Jolt, Kinematic)を今の姿勢へ合わせる(Update)
//
// ボディは Start(RegisterPhysicsBodySystem)で作って持ち続け、ここで位置・向き・拡大率だけ動かす。
//
// 判定クエリ(Physics フェーズ)が見る姿勢は、ここで合わせた Update フェーズ時点のもの。
// 位置の積分(Physics フェーズ)より前なので1フレーム遅れるが、弾も同じ条件なので実用上問題ない
//==========================================================================================
class SyncPhysicsBodySystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
