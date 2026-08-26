#pragma once
#include "Application/ECS/ISystem/ISystem.h"

//==============================================================================
// FollowAnimationNodeSystem
//  HierarchyComponent で紐づいた親モデルの、指定アニメーションノードの
//  ワールド行列に自分のLocalTransformを追従させるシステム。
//==============================================================================
class FollowAnimationNodeSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
