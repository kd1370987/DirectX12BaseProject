#pragma once
#include "Application/ECS/ISystem/APPISystem.h"

//==============================================================================
// FollowAnimationNodeSystem
//  HierarchyComponent で紐づいた親モデルの、指定アニメーションノードの
//  ワールド行列に自分のLocalTransformを追従させるシステム。
//==============================================================================
class FollowAnimationNodeSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
