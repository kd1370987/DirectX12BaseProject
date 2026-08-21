#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

// 本体が武器を兼ねているキャラ(銃を子として持たず、自分に GunStateComponent がある敵など)の
// 引き金を、自分の ActionIntentComponent から自分の WeaponTriggerComponent へ渡すシステム。
// 武器が子エンティティの場合に配信する AttachmentDispatchSystem と対になる。
class SelfWeaponTriggerSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
