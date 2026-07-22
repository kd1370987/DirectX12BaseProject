#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

//==========================================================================================
// AttachmentDispatchSystem
//
// プレイヤーが持つ AttachmentSlotsComponent を辿り、
// プレイヤーの入力・状態を、各スロットに割り当てられた子エンティティへ配信する。
//   - ブースター : ブースト状態 -> 子の ParticlesComponent.isPlay
//   - 銃         : 発射入力     -> 子の ActionIntentComponent.isGunShoot / isAiming
//
// 実際の噴射(EmittParticleSystem)や発射(GunShootSystem)は、
// 子エンティティ側の既存システムがそのまま処理する。
//==========================================================================================
class AttachmentDispatchSystem : public Engine::ECS::SystemBase<AttachmentDispatchSystem>
{
public:

	static constexpr Engine::ECS::ESystemType s_type = Engine::ECS::ESystemType::PreUpdate;
	void Init(Engine::ECS::World& a_world) override;
};
