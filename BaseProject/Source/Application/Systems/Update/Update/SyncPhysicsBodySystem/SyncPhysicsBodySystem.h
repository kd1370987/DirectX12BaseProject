#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// 動くコライダーのボディ(Jolt, Kinematic)を今の姿勢へ合わせる(Update)
//
// 旧 CollisionWorld は動的ワールドを毎フレーム作り直していた(SubmitDynamicColliderSystem)。
// Jolt ではボディを Start で作って持ち続け、ここで位置・向き・拡大率だけ動かす。
//
// 置き場所は SubmitDynamicColliderSystem と同じ Update フェーズ。
// 判定クエリ(Physics フェーズ)が見る姿勢が旧と同じタイミング(1フレーム遅れ)になる
//==========================================================================================
class SyncPhysicsBodySystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
