#pragma once
#include "Engine/ECS/System/SystemBase/SystemBase.h"

//==============================================================================
// FollowAnimationNodeSystem
//  HierarchyComponent で紐づいた親モデルの、指定アニメーションノードの
//  ワールド行列に自分のLocalTransformを追従させるシステム。
//==============================================================================
class FollowAnimationNodeSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
