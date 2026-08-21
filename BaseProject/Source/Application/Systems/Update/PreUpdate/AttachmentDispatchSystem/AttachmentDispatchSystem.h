#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

//==========================================================================================
// AttachmentDispatchSystem
//
// プレイヤーが持つ AttachmentSlotsComponent を辿り、
// プレイヤーの入力・状態を、各スロットに割り当てられた子エンティティへ配信する。
//   - ブースター : ブースト状態 -> 子の ParticlesComponent.isPlay
//   - 左右の武器 : 引き金       -> 子の WeaponTriggerComponent.isPulled
//
// 渡すのは命令だけ。実際の噴射(EmitParticleSystem)や
// 「今撃てるのか・何をどう撃つのか」(GunShootSystem + GunStateComponent)は、
// 子エンティティ側がすべて自分で判断する。
//==========================================================================================
class AttachmentDispatchSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
