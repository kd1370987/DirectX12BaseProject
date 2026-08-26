#pragma once

#include "Application/ECS/ISystem/ISystem.h"

//==========================================================================================
// SubmitDynamicColliderSystem
//
// Layer::DiynamicObject のコライダーを毎フレーム動的コリジョンワールドへ submit する。
// (RegisterCollisionWorldSystem は Start 時に静的コライダーのみ登録し、
//  動的レイヤーはスキップしているため、動くコライダーはこのシステムが担当する)
//
// フレーム順序:
//   MainEngine::BeginFrame で ClearDynamicWorld ->
//   Update フェーズで本システムが AllcateDynamicEntity ->
//   BaseScene::Update が Update と Physics の間で BuildDynamicWorld ->
//   Physics フェーズのクエリ(HitDetectSystem 等)が静的+動的の両TLASを走査。
//==========================================================================================
class SubmitDynamicColliderSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
