#include "SelfWeaponTriggerSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Character/Weapon/WeaponTriggerComponent.h"

//==========================================================================================
// SelfWeaponTriggerSystem
//
// 「持ち主の命令」を「武器への引き金」へ変える経路のうち、本体が武器を兼ねている場合を担当する。
// 武器を子エンティティとして持つ場合は AttachmentDispatchSystem が同じことをする。
//
// このクエリ(ActionIntent と WeaponTrigger の両方を持つ)に引っかかるのは、
// 自分自身に GunStateComponent がある敵のようなキャラだけ。
// 武器エンティティ側は ActionIntentComponent を持たない(命令を出す側ではないので)ため、
// 配信された引き金をここで踏み消してしまうことはない。
//
// 左右の区別は付けない。両手を持たないキャラに「どちらの手か」を決めさせても意味が無いので、
// どちらかが押されていれば引く、とする。
//==========================================================================================
void SelfWeaponTriggerSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActionIntentComponent, WeaponTriggerComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"SelfWeaponTriggerSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const ActionIntentComponent*      a_intentArray,
			WeaponTriggerComponent*           a_triggerArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				a_triggerArray[_i].isPulled = a_intentArray[_i].IsAnyWeaponShoot();
			}
		}
	);
}
