#pragma once

#include "Application/ECS/ISystem/ISystem.h"

//==========================================================================================
// 追従ノード解決の待ちゲート
//
// FollowAnimationNodeComponent を持つエンティティは、AttachmentNodeLinkSystem(Start) で
// 「親モデルのノード名ハッシュ」から追従先のノード番号を解決する。
// このとき見るのは自分ではなく親のモデルなので、
// ModelReadyGateSystem(自分のモデルを見る)だけでは取りこぼす。
//
// 親モデルが読込中だと ResourceManager::Get() は
// 「中身が空のモデルへの有効なポインタ」を返すため、nullチェックでは弾けない。
// ノード配列が空のまま検索が走って一致がなく、
// ノード番号が既定値(0)のまま残る = 別のノードに張り付く。
//
// そこで親モデルが揃うまでこのエンティティを Start へ進めない
//==========================================================================================
class AttachmentReadyGateSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};
