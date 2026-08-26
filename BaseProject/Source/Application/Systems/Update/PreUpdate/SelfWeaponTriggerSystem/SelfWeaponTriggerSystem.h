#pragma once

#include "Application/ECS/ISystem/ISystem.h"

// 本体が武器を兼ねているキャラ(銃を子として持たず、自分に GunStateComponent がある敵など)の
// 引き金を、自分の ActionIntentComponent から自分の WeaponTriggerComponent へ渡すシステム。
// 武器が子エンティティの場合に配信する AttachmentDispatchSystem と対になる。
class SelfWeaponTriggerSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
