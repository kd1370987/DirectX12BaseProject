#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

// クリップ適用後・ワールド行列計算前に、ノードのローカル行列へ加算回転を合成する。
//
// 実行順が重要:
//   AnimationSystem(バインドポーズリセット + クリップ適用)
//     ↓
//   AdditivePoseSystem   ← ここ
//     ↓
//   CalcNodeSystem(local → world)
//
// AnimationSystem より前で書き込むとバインドポーズリセットで消える。
class AdditivePoseSystem : public App::ECS::APPISystem
{
public:


	void Init(App::ECS::APPWorld& a_world) override;
};
